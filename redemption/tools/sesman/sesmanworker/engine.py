from wallix.logger import Logger

from wabengine.common.exception import (
    LicenseException,
    SessionAlreadyStopped,
)
from wallixgenericnotifier import (
    Notify,
    CX_EQUIPMENT,
    PATTERN_FOUND,
    PRIMARY_CX_FAILED,
    SECONDARY_CX_FAILED,
    NEW_FINGERPRINT,
    WRONG_FINGERPRINT,
    RDP_PATTERN_FOUND,
    RDP_PROCESS_FOUND,
    RDP_OUTCXN_FOUND,
    FILESYSTEM_FULL,
)
from wabconfig import Config
from wallixconst.protocol import RDP, VNC  # noqa: F401
from wallixconst.authentication import (  # noqa: F401
    PASSWORD_VAULT,
    PASSWORD_INTERACTIVE,
    PASSWORD_MAPPING,
)
from wallixconst.account import AM_IL_DOMAIN
from wallixconst.trace import LOCAL_TRACE_PATH_RDP
from wallixredis import redis
from wallixconst.configuration import IS_CLOUD_CONFIGURATION
from typing import Optional, Union, Tuple, Dict, List, Any, Iterable, NamedTuple, Protocol

from .logtime import logtime_function_pause
import time
import socket
import traceback
from .wallixauth import Authenticator
from .challenge import Challenge
from .checkout import CheckoutEngine, AccountInfos, AccountType
from .checkout import (
    APPROVAL_ACCEPTED,
    APPROVAL_REJECTED,
    APPROVAL_PENDING,
    APPROVAL_NONE,
    SHADOW_ACCEPTED,
    SHADOW_REJECTED,
)
from .transaction import manage_transaction
from . import targetaccuratefilter as taf
from .addrutils import (
    is_device_in_subnet,
    is_ip_address,
    resolve_reverse_dns,
)


CRED_DATA_LOGIN = "login"
CRED_DATA_ACCOUNT_UID = "account_uid"
CRED_INDEX = "credentials"

APPREQ_REQUIRED = 1
APPREQ_OPTIONAL = 0

SOCK_PATH_DIR = "/var/run/redemption"

DEFAULT_CONF_DIR = "/var/wab/etc/"
DEFAULT_SPEC_DIR = "/opt/wab/share/conf/"

FINGERPRINT_SHA1 = 0
FINGERPRINT_MD5 = 1
FINGERPRINT_MD5_LEN = 16
FINGERPRINT_SHA1_LEN = 20

RightType = Dict[str, Any]
Protocols = List[str]


class TimeCtx(Protocol):
    @property
    def timezone(self) -> int: ...
    @property
    def altzone(self) -> int: ...
    @property
    def daylight(self) -> int: ...

    def time(self) -> float: ...
    def mktime(self, struct_time) -> float: ...


class TargetContext:
    def __init__(self, host=None, dnsname=None, login=None, service=None,
                 group=None, show=None):
        self.host = host
        self.dnsname = dnsname
        self.login = login
        self.service = service
        self.group = group
        self.show = show
        self.strict_transparent = False

    def showname(self) -> str:
        return self.show or self.dnsname or self.host

    def is_empty(self) -> bool:
        return not (self.host or self.login or self.service or self.group)


class ProtocolInfo:
    __slots__ = (
        "protocol", "subprotocols",
    )

    def __init__(self, protocol: str, subprotocols: Protocols = []):
        self.protocol = protocol
        self.subprotocols = subprotocols


class ExtraInfo:
    __slots__ = (
        "has_approval",
        "is_critical",
        "is_recorded",
    )

    def __init__(self, is_recorded: bool, is_critical: bool, has_approval: bool):
        self.is_recorded = is_recorded
        self.is_critical = is_critical
        self.has_approval = has_approval


class PhysicalTarget:
    __slots__ = (
        "account_login",
        "device_host",
        "device_id",
        "service_port",
        "sharing_host",
    )

    def __init__(self, device_host: str, account_login: str, service_port: int, device_id: str,
                 sharing_host: Optional[str] = None):
        self.device_host = device_host
        self.account_login = account_login
        self.service_port = service_port
        self.device_id = device_id
        self.sharing_host = sharing_host


class LoginInfo:
    __slots__ = (
        "account_login",
        "account_name",
        "auth_name",
        "conn_opts",
        "device_host",
        "domain_name",
        "service_name",
        "service_port",
        "target_group_name",
        "target_name",
        "user_group_name",
    )

    def __init__(self, account_login: str, account_name: str, domain_name: str, target_name: str,
                 service_name: str, auth_name: str, user_group_name: str, target_group_name: str,
                 device_host: str, service_port: int, conn_opts: Dict[str, Any]):
        self.account_login = account_login
        self.account_name = account_name
        self.domain_name = domain_name
        self.target_name = target_name
        self.service_name = service_name
        self.auth_name = auth_name
        self.user_group_name = user_group_name
        self.target_group_name = target_group_name
        self.device_host = device_host
        self.service_port = service_port
        self.conn_opts = conn_opts

    def get_target_str(self) -> str:
        domain_name = f"@{self.domain_name}" if self.domain_name else ''
        return f"{self.account_name}{domain_name}@{self.target_name}:{self.service_name}:{self.auth_name}"

    def get_target_dict(self) -> Dict[str, str]:
        return {
            "user_group": self.user_group_name,
            "target_group": self.target_group_name,
            "authorization": self.auth_name,
            "account": self.account_name,
            "account_domain": self.domain_name,
            "device": self.target_name,
            "service": self.service_name,
        }


class AppParams(NamedTuple):
    program: str
    params: Optional[str]
    workingdir: Optional[str]


def read_config_file(modulename="sesman",
                     confdir=DEFAULT_CONF_DIR,
                     specdir=DEFAULT_SPEC_DIR):
    return Config(modulename=modulename, confdir=confdir, specdir=specdir)


def decode_rawtext_data(data: bytes, default_msg: str) -> str:
    for encoding in ('utf-8', 'iso-8859-1'):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return default_msg


def _read_message(filename: str, default_msg: str, format_terminal: bool) -> str:
    try:
        with open(filename, "rb") as f:
            msg = decode_rawtext_data(f.read(), default_msg)
    except IOError:
        msg = default_msg

    if format_terminal:
        msg += '\n'
        msg = msg.replace('\n', '\r\n')
    return msg


class Engine:
    def __init__(self):
        self.wabengine = None
        self.checkout = None
        self.user_cn = None
        self.user_lang = None
        self.wabengine_conf = Config('wabengine')
        self.authenticator = Authenticator()
        self.session_id = None
        self._selector_banner = {}
        self.session_record = None
        self.session_record_type = None
        self.deconnection_epoch = 0xffffffff
        self.deconnection_time = "-"
        self.start_time = None

        self.proxy_rights = None
        self.rights = None
        self.targets = {}
        self.targetsdom = {}
        self.target_right = None

        self.displaytargets = []

        self.session_result = True
        self.session_diag = 'Success'
        self.trace_hash = None
        self.failed_secondary_set = False

        self.service = None

        self.checktarget_cache = None
        self.checktarget_infos_cache = None

        self.avatar_id = None

        self.authenticated = False

    def keepalive(self, timeout: int) -> None:
        if self.avatar_id:
            with manage_transaction(self.wabengine):
                self.wabengine.save_session(self.avatar_id, timeout=timeout)

            if self.session_id:
                with manage_transaction(self.wabengine):
                    self.wabengine.keepalive_session(
                        self.session_id,
                        timeout=timeout
                    )

    def _post_authentication(self) -> None:
        self.avatar_id = self.wabengine.connect(timeout=60)
        self.update_user()
        self.checkout = CheckoutEngine(self.wabengine)
        self.authenticated = True

    def update_user(self) -> None:
        if not self.wabengine:
            return
        with manage_transaction(self.wabengine):
            userobj = self.wabengine.who_am_i()
            self.user_cn = userobj.cn
            self.user_lang = userobj.preferredLanguage

    def get_session_status(self) -> Tuple[bool, str]:
        return self.session_result, self.session_diag

    # NOTE [RDP] calls to set_session_status() always initialize diag
    def set_session_status(self, diag: str, result: Optional[bool] = None) -> None:
        # Logger().info("Engine set session status : result='{result}', diag='{diag}'")
        if result is not None:
            self.session_result = result
        self.session_diag = diag

    def get_language(self) -> str:
        if not self.user_lang:
            self.update_user()
        return self.user_lang or 'en'

    def get_username(self) -> str:
        if not self.user_cn:
            self.update_user()
        return self.user_cn or ""

    def get_otp_client(self) -> str:
        try:
            with manage_transaction(self.wabengine):
                return self.wabengine.get_otp_client()
        except Exception:
            return ''

    def check_license(self) -> bool:
        with manage_transaction(self.wabengine):
            res = self.wabengine.is_session_management_license()
        if not res:
            Logger().info("License Error: "
                          "Session management License is not available.")
        return res

    # NOTE [RDP] calls to get_banner() always initialize lang
    def get_banner(self, lang: str = '', format_terminal: bool = False) -> str:
        banner = ("Warning! Your remotée session may be recorded and "
                  "kept in electronic format.")
        return _read_message(f'/var/wab/etc/proxys/messages/motd.{lang}',
                             banner, format_terminal)

    # NOTE [RDP] calls to get_warning_message() always initialize lang
    def get_warning_message(self, lang: str = '', format_terminal: bool = False):
        if lang == 'fr':
            msg = ("Attention! Toute tentative d'acces sans"
                   " autorisation ou de maintien frauduleux"
                   " dans ce systeme fera l'objet de"
                   " poursuites judiciaires.")
        else:
            msg = ("Warning! Unauthorizedé access to this system is"
                   " forbidden and will be prosecuted by law.")
        return _read_message(f'/var/wab/etc/proxys/messages/login.{lang}',
                             msg, format_terminal)

    def get_deconnection_time_msg(self, lang: str) -> str:
        message = ""
        if self.deconnection_time and self.deconnection_time != '-':
            if lang == 'fr':
                message = ("\033[1mAttention\033[0m: Cette connexion sera "
                           f"coupée à {self.deconnection_time}.\r\n")
            else:
                message = ("\033[1mWarning\033[0m: This connection will "
                           f"be closed at {self.deconnection_time}.\r\n")
        return message

    def init_timeframe(self, auth: Dict[str, Any]) -> None:
        deconnection_time = auth['deconnection_time']
        if (deconnection_time
            and deconnection_time != "-"
            and deconnection_time[0:4] <= "2034"
        ):
            self.deconnection_time = deconnection_time
            self.deconnection_epoch = int(
                time.mktime(
                    time.strptime(deconnection_time, "%Y-%m-%d %H:%M:%S")
                )
            )

    def get_trace_type(self) -> str:
        try:
            return self.wabengine_conf.get('trace', 'localfile_hashed')
        except Exception:
            Logger().info("Engine get_trace_type failed: "
                          "configuration file section "
                          "'wabengine', key 'trace', "
                          f"((({traceback.format_exc()})))")
        return 'localfile_hashed'

    def get_selector_banner(self) -> Dict[str, Any]:
        try:
            self._selector_banner = self.wabengine_conf.get('banner')
        except Exception:
            Logger().info("Engine get_selector_banner failed: "
                          "configuration file section "
                          "'wabgine', key 'trace', "
                          f"((({traceback.format_exc()})))")
        return self._selector_banner

    def get_trace_encryption_key(self, path: str, old_scheme: bool = False) -> None:
        with manage_transaction(self.wabengine):
            return self.wabengine.get_trace_encryption_key(path, old_scheme)

    def get_trace_sign_key(self) -> None:
        with manage_transaction(self.wabengine):
            return self.wabengine.get_trace_sign_key()

    def password_expiration_date(self) -> Tuple[bool, int]:
        try:
            with manage_transaction(self.wabengine):
                _data = self.wabengine.check_password_expiration_info()
            if _data[2]:
                Logger().info(f"Engine password_expiration_date={_data[0]}")
                return True, _data[0]
        except Exception:
            Logger().info("Engine password_expiration_date failed: "
                          f"((({traceback.format_exc()})))")
        return False, 0

    def is_x509_connected(self, wab_login: str, ip_client: str, proxy_type: str, target: str,
                          ip_server: str) -> bool:
        return self.authenticator.is_x509_connected(
            wab_login, ip_client, proxy_type, target, ip_server
        )

    def is_x509_validated(self) -> bool:
        return self.authenticator.is_x509_validated()

    @logtime_function_pause
    def x509_authenticate(self) -> bool:
        return self.authenticator.x509_authenticate(self)

    def mobile_device_authenticate(self) -> bool:
        return self.authenticator.mobile_device_authenticate(self)

    def url_redirect_authenticate(self) -> bool:
        return self.authenticator.url_redirect_authenticate(self)

    def check_kbdint_auth(self, wab_login: str, ip_client: str, ip_server: str) -> Union[bool, str, None]:
        return self.authenticator.check_kbdint_authenticate(
            self, wab_login, ip_client, ip_server
        )

    def password_authenticate(self, wab_login: str, ip_client: str, password: str, ip_server: str) -> bool:
        return self.authenticator.password_authenticate(
            self, wab_login, ip_client, password, ip_server
        )

    def passthrough_authenticate(self, wab_login: str, ip_client: str, ip_server: str) -> bool:
        return self.authenticator.passthrough_authenticate(
            self, wab_login, ip_client, ip_server
        )

    def gssapi_authenticate(self, wab_login: str, ip_client: str, ip_server: str) -> bool:
        return self.authenticator.gssapi_authenticate(
            self, wab_login, ip_client, ip_server
        )

    def pubkey_authenticate(self, wab_login: str, ip_client: str, ip_server: str,
                            pubkey, ca_pubkey) -> bool:
        return self.authenticator.pubkey_authenticate(
            self, wab_login, ip_client, ip_server, pubkey, ca_pubkey
        )

    def get_challenge(self) -> Optional[Challenge]:
        return self.authenticator.get_challenge()

    def reset_challenge(self) -> None:
        self.authenticator.reset_challenge()

    def resolve_target_host(self,
                            target_device: str,
                            target_login: str,
                            target_service: str,
                            target_group: str,
                            real_target_device: str,
                            target_context: Optional[TargetContext],
                            passthrough_mode: bool,
                            protocols: Protocols
                            ) -> Tuple[Optional[str], Optional[TargetContext]]:
        """ Resolve the right target host to use
        target_context.host will contains the target host.
        target_context.showname() will contains the target_device to show
        target_context.login will contains the target_login if not in
            passthrough mode.

        Returns target_device: None target_device is a hostname,
                target_context
        """
        if real_target_device:
            # Transparent proxy
            if not target_context or not target_context.host:
                dnsname = resolve_reverse_dns(real_target_device)
                target_context = TargetContext(
                    host=real_target_device,
                    dnsname=dnsname
                )
                target_context.strict_transparent = True
        elif target_device:
            # This allow proxy to check if target_device is a device_name
            # or a hostname.
            # In case it is a hostname, we keep the target_login as a filter.
            valid = self.valid_device_name(protocols, target_device)
            Logger().info(f"Check Valid device '{target_device}' : res = {valid}")
            if not valid:
                # target_device might be a hostname
                try:
                    login_filter, service_filter, group_filter = (
                        None, None, None
                    )
                    dnsname = None
                    if (target_login and not passthrough_mode):
                        login_filter = target_login
                    if (target_service and not passthrough_mode):
                        service_filter = target_service
                    if (target_group and not passthrough_mode):
                        group_filter = target_group
                    host_ip = socket.getaddrinfo(target_device, None)[0][4][0]
                    Logger().info(f"Resolve DNS Hostname {target_device} -> {host_ip}")
                    if not is_ip_address(target_device):
                        dnsname = target_device
                    target_context = TargetContext(host=host_ip,
                                                   dnsname=dnsname,
                                                   login=login_filter,
                                                   service=service_filter,
                                                   group=group_filter,
                                                   show=target_device)
                    target_device = None
                except Exception:
                    # Logger().info(f"resolve_hostname: ((({traceback.format_exc()})))")
                    Logger().info("target_device is not a hostname")
        return target_device, target_context

    def NotifyConnectionToCriticalEquipment(self, protocol: str, user: str,
                                            source: str, ip_source: str,
                                            login: str, device: str, ip: str,
                                            time: str, url: Optional[str]) -> None:
        try:
            notif_data = {
                'protocol': protocol,
                'user': user,
                'source': source,
                'ip_source': ip_source,
                'login': login,
                'device': device,
                'ip': ip,
                'time': time
            }

            if url is not None:
                notif_data['url'] = url

            Notify(self.wabengine, CX_EQUIPMENT, notif_data)
        except Exception:
            Logger().info("Engine NotifyConnectionToCriticalEquipment failed: "
                          f"((({traceback.format_exc()})))")

    def NotifySecondaryConnectionFailed(self, user: str, ip: str, account: str,
                                        device: str) -> None:
        try:
            notif_data = {
                'user': user,
                'ip': ip,
                'account': account,
                'device': device,
            }

            Notify(self.wabengine, SECONDARY_CX_FAILED, notif_data)
        except Exception:
            Logger().info("Engine NotifySecondaryConnectionFailed failed: "
                          f"((({traceback.format_exc()})))")

    def NotifyFilesystemIsFullOrUsedAtXPercent(self, filesystem: str,
                                               used: str) -> None:
        try:
            notif_data = {
                'filesystem': filesystem,
                'used': used
            }

            Notify(self.wabengine, FILESYSTEM_FULL, notif_data)
        except Exception:
            Logger().info("Engine NotifyFilesystemIsFullOrUsedAtXPercent "
                          f"failed: ((({traceback.format_exc()})))")

    def NotifyFindPatternInRDPFlow(self, regexp: str, string: str,
                                   user_login: str, user: str, host: str,
                                   cn: str, service: str) -> None:
        try:
            notif_data = {
                'regexp': regexp,
                'string': string,
                'user_login': user_login,
                'user': user,
                'host': host,
                'device': cn,
                'service': service,
            }

            Notify(self.wabengine, RDP_PATTERN_FOUND, notif_data)

            Logger().info(
                f"{regexp}: The string '{string}' has been detected in the "
                "following RDP connection: "
                f"{user}@{cn}:{service}:{user_login} ({host})\n"
            )
        except Exception:
            Logger().info("Engine NotifyFindPatternInRDPFlow failed: "
                          f"((({traceback.format_exc()})))")

    def notify_find_connection_rdp(self, rule: str, deny: bool, app_name: str,
                                   app_cmd_line: str, dst_addr: str,
                                   dst_port: str, user_login: str, user: str,
                                   host: str, cn: str, service: str) -> None:
        try:
            notif_data = {
                'rule': rule,
                'deny': deny,
                'app_name': app_name,
                'app_cmd_line': app_cmd_line,
                'dst_addr': dst_addr,
                'dst_port': dst_port,
                'user_login': user_login,
                'user': user,
                'host': host,
                'device': cn,
                'service': service
            }
            Notify(self.wabengine, RDP_OUTCXN_FOUND, notif_data)
            Logger().info(
                f"{rule}: The connection '{dst_addr}:{dst_port}' "
                "has been detected in the following RDP connection: "
                f"{user}@{cn}:{service}:{user_login} ({host})\n"
            )
        except Exception:
            Logger().info("Engine NotifyFindConnectionInRDPFlow failed: "
                          f"((({traceback.format_exc()})))")

    def notify_find_process_rdp(self, regex: str, deny: bool, app_name: str,
                                app_cmd_line: str, user_login: str, user: str,
                                host: str, cn: str, service: str) -> None:
        try:
            notif_data = {
                'regex': regex,
                'deny': deny,
                'app_name': app_name,
                'app_cmd_line': app_cmd_line,
                'user_login': user_login,
                'user': user,
                'host': host,
                'device': cn,
                'service': service
            }
            Notify(self.wabengine, RDP_PROCESS_FOUND, notif_data)
            Logger().info(
                f"{regex}: The application '{app_name}' has been detected "
                "in the following RDP connection: "
                f"{user}@{cn}:{service}:{user_login} ({host})\n"
            )
        except Exception:
            Logger().info("Engine NotifyFindProcessInRDPFlow failed: "
                          f"((({traceback.format_exc()})))")

    def get_targets_list(self, group_filter: str, device_filter: str, protocol_filter: str,
                         case_sensitive: bool) -> Tuple[List[Tuple[str, str, str]], bool]:
        fc = (lambda string: string) if case_sensitive else (lambda string: string.lower())

        targets = []
        selector_filter_mode = taf.get_selector_filter_mode(device_filter)

        if selector_filter_mode == taf.SelectorFilterMode.ADVANCED:
            try:
                filter_pattern_dict = (
                    taf.get_filter_pattern_dict(fc(device_filter)))
            except RuntimeError:
                return targets, True

        item_filtered = False

        for target_info in self.displaytargets:
            temp_service_login = target_info.service_login
            temp_resource_service_protocol_cn = target_info.protocol

            if (fc(group_filter) not in fc(target_info.group)
                or fc(protocol_filter) not in fc(temp_resource_service_protocol_cn)):
                item_filtered = True
                continue

            if selector_filter_mode == taf.SelectorFilterMode.NORMAL:
                # apply target global filter mode

                if fc(device_filter) not in fc(temp_service_login):
                    item_filtered = True
                    continue
            elif selector_filter_mode == taf.SelectorFilterMode.ADVANCED:
                # apply target accurate filter mode

                target_login_real, sep, target_domain_real = (
                    target_info.target_login.rpartition('@'))

                if not sep:
                    target_login_real, target_domain_real = (
                        target_domain_real, "")

                target_device = target_info.target_name
                target_service = target_info.service_name
                target_field_dict = {
                    "account": fc(target_login_real),
                    "domain": fc(target_domain_real),
                    "device": fc(target_device),
                    "service": fc(target_service)}

                if not taf.is_filterable(filter_pattern_dict,
                                         target_field_dict):
                    item_filtered = True
                    continue

            targets.append((target_info.group,  # ( = concatenated list)
                            temp_service_login,
                            temp_resource_service_protocol_cn))
        # Logger().info(f"targets list = {targets}'")
        return targets, item_filtered

    def reset_proxy_rights(self) -> None:
        """
        Clean current fetched rights
        """
        self.proxy_rights = None
        self.rights = None
        self.target_right = None
        self.release_all_target()
        self.reset_target_session()

    def reset_target_session(self) -> None:
        """
        Clean Target session (if exist) rights and infos
        """

        self.target_right = None
        self.deconnection_epoch = 0xffffffff
        self.deconnection_time = "-"
        self.start_time = None

        self.session_record = None
        self.session_record_type = None
        self.session_id = None
        self.session_result = True
        self.session_diag = 'Success'
        self.failed_secondary_set = False
        self.trace_hash = None

        self.service = None

        self.checktarget_cache = None
        self.checktarget_infos_cache = None

    def proxy_session_logout(self) -> None:
        """
        Clean all information of current authenticated proxy session
        """
        self.reset_proxy_rights()
        if self.authenticator is not None:
            self.authenticator.reset()
        self.close_client()
        self.user_cn = None
        self.authenticated = False

    def close_client(self) -> None:
        if self.wabengine:
            Logger().debug("Engine close_client()")
            with manage_transaction(self.wabengine, close=True):
                if self.avatar_id:
                    self.wabengine.disconnect(self.avatar_id)
            self.wabengine = None
        self.avatar_id = None

    def get_proxy_user_rights(self, protocols: Protocols, target_device: str) -> List[RightType]:
        Logger().debug(f"** CALL Get_proxy_right ** proto={protocols}, target_device={target_device}")
        with manage_transaction(self.wabengine):
            urights = self.wabengine.get_proxy_user_rights(protocols,
                                                           target_device)
        Logger().debug("** END Get_proxy_right **")
        if urights and isinstance(urights[0], str):
            import json
            urights = map(json.loads, urights)
        return urights

    def valid_device_name(self, protocols: Protocols, target_device: str) -> bool:
        try:
            # Logger().debug("** CALL VALIDATOR DEVICE NAME "
            #                f"Get_proxy_right target={target_device} **")
            prights = self.get_proxy_user_rights(protocols, target_device)
            # Logger().debug("** END VALIDATOR DEVICE NAME Get_proxy_right **")
            if prights:
                self.proxy_rights = prights
                return True
        except Exception:
            # Logger().info(f"valid_device_name failed: ((({traceback.format_exc()})))")
            pass
        return False

    def _filter_rights(self, target_context: Optional[TargetContext]) -> None:
        from collections import defaultdict
        self.rights = self.proxy_rights
        # targets{(account, target)}{domain}[(service, group, right)]
        self.targets = defaultdict(lambda: defaultdict(list))
        self.targets_alias = defaultdict(lambda: defaultdict(list))
        self.displaytargets = []
        for right in self.rights:
            account_name = right['account_name']
            account_domain = right['domain_cn']
            account_login = right['account_login']
            account_logindom = self.get_account_login(right,
                                                      check_in_creds=False)
            account_namedom = account_name
            if account_domain and account_domain != AM_IL_DOMAIN:
                account_namedom = f"{account_name}@{account_domain}"
            try:
                auth_name = right['auth_cn']
            except Exception:
                auth_name = right['target_group']
            if right['application_cn']:
                target_name = right['application_cn']
                service_name = "APP"
                protocol = "APP"
                host = None
                alias = None
                subprotocols = []
            else:
                target_name = right['device_cn']
                service_name = right['service_cn']
                protocol = right['service_protocol_cn']
                host = right['device_host']
                alias = right['device_alias']
                subprotocols = right['service_subprotocols']
            if target_context is not None:
                if target_context.host and host is None:
                    continue
                if (target_context.host
                    and not is_device_in_subnet(target_context.host, host)
                    and host != target_context.dnsname):
                    continue
                if (target_context.login
                    and account_login
                    and target_context.login not in (
                        account_login, account_logindom,
                        account_name, account_namedom)):
                    # match context login with login or name
                    # (with or without domain)
                    continue
                if (target_context.service
                    and service_name != target_context.service):
                    continue
                if (target_context.group
                    and target_context.group != auth_name):
                    continue

            target_value = (service_name, auth_name, right)
            # feed targets hashtable indexed on account_name and target_name
            # targets{(account, target)}{domain}[(service, group, right)]
            tuple_index = (account_name, target_name)
            self.targets[tuple_index][account_domain].append(target_value)
            if alias:
                alias_index = (account_name, alias)
                self.targets_alias[alias_index][account_domain].append(
                    target_value
                )

            self.displaytargets.append(DisplayInfo(account_namedom,
                                                   target_name,
                                                   service_name,
                                                   protocol,
                                                   auth_name,
                                                   subprotocols,
                                                   host))
        if target_context and target_context.strict_transparent:
            self._filter_subnet()

    def _filter_subnet(self) -> None:
        # in transparent mode, targets rights are already
        # filtered by a unique host
        filtered_subnet = [dit for dit in self.displaytargets
                           if '/' not in dit.host]
        if filtered_subnet:
            self.displaytargets = filtered_subnet

    def filter_app_rights(self, app_rights: Iterable[RightType],
                          account_name: str, domain_name: str, app_name: str) -> List[RightType]:
        return [r for r in app_rights if (
            r['account_name'] == account_name
            and (not domain_name or r['domain_cn'] == domain_name)
            and r['application_cn'] == app_name)]

    def get_proxy_rights(self, protocols: Protocols, target_device: Optional[str] = None,
                         target_context: Optional[TargetContext] = None) -> None:
        if self.proxy_rights is None:
            try:
                self.proxy_rights = self.get_proxy_user_rights(
                    protocols, target_device)
                # Logger().debug("** END Get_proxy_right **")
            except Exception:
                # Logger().info(f"traceback = {traceback.format_exc()}")
                self.proxy_rights = None
                return
        if self.rights is not None:
            return
        # start = time.time()
        # Logger().debug("** BEGIN Filter_rights **")
        self._filter_rights(target_context)
        # Logger().debug(f"** END Filter_rights in {time.time() - start} sec **")

    def _get_target_right_htable(self, target_account: str, target_device: str,
                                 t_htable: Dict[Tuple[str, str], Dict[str, List[Any]]]) -> List[Tuple[Any, Any, Any]]:
        """
        Get target right list from t_htable
        filtered by target_account and target_device

        target_account = <login>@<domain> or <login>
        '@' might be present in login but not in domain
        t_htable = {(account, device)}{domain}[(service, group, right)]
        device can be an alias
        """
        try:
            acc_dom = target_account.rsplit('@', 1)
            account = acc_dom[0]
            domain = acc_dom[1] if len(acc_dom) == 2 else ''
            domres = t_htable.get((account, target_device), {})
            results = domres.get(domain)
            if not results:
                # domain might not be provided in target_account
                if domain:
                    # domain field is not empty so try again
                    # with target_account
                    domres = t_htable.get((target_account, target_device), {})
                if len(domres) == 1:
                    # no ambiguity, get first (and only) value
                    results = domres[next(iter(domres))]
                else:
                    # ambiguity on domain
                    results = []
        except Exception:
            results = []
        return results

    def _find_target_right(self, target_account: str, target_device: str,
                           target_service: str, target_group: str) -> Optional[RightType]:
        try:
            Logger().debug(f"Find target {target_account}@{target_device}:{target_service}:{target_group}")
            results = self._get_target_right_htable(
                target_account, target_device, self.targets)
            if not results:
                results = self._get_target_right_htable(
                    target_account, target_device, self.targets_alias)
        except Exception:
            results = []
        right = None
        filtered = [
            (r_service, r)
            for (r_service, r_groups, r) in results if (
                ((not target_service) or (r_service == target_service))
                and ((not target_group) or (r_groups == target_group))
            )
        ]
        if filtered:
            filtered_service, right = filtered[0]
            # if ambiguity in group but not in service,
            # get the right without approval
            for (r_service, r) in filtered[1:]:
                if filtered_service != r_service:
                    right = None
                    break
                if r['auth_has_approval'] is False:
                    right = r
            if right:
                self.init_timeframe(right)
                self.target_right = right
        return right

    def get_target_rights(self, target_login: str, target_device: str, target_service: str,
                          target_group: str, target_context: Optional[TargetContext] = None
                          ) -> Tuple[Optional[RightType], str]:
        try:
            self.get_proxy_rights(['SSH', 'TELNET', 'RLOGIN', 'RAWTCPIP'],
                                  target_device=target_device,
                                  target_context=target_context)
            right = self._find_target_right(target_login, target_device,
                                            target_service, target_group)
            if right:
                return right, "OK"

            Logger().error(f"Bastion account {self.user_cn} couldn't log "
                           f"into {target_login}@{target_device}:{target_service}")
        except Exception:
            Logger().info(f"traceback = {traceback.format_exc()}")

        target_str = f"{target_login}@{target_device}:{target_service} ({target_group})"
        msg = (f"Cible {target_str} invalide\r\n"
               if self.get_language() == "fr"
               else f"Invalid target {target_str}\r\n")
        return (None, msg)

    def get_selected_target(self, target_login: str, target_device: str, target_service: str,
                            target_group: str, target_context: Optional[TargetContext] = None
                            ) -> Optional[RightType]:
        # Logger().info(
        #     f">>==GET_SELECTED_TARGET {target_device}@{target_login}:{target_service}:{target_group}"
        # )
        self.get_proxy_rights(['RDP', 'VNC'], target_device,
                              target_context=target_context)
        return self._find_target_right(target_login, target_device,
                                       target_service, target_group)

    def get_effective_target(self, selected_target: RightType) -> List[RightType]:
        application = selected_target['application_cn']
        try:
            if application and not self.is_sharing_session(selected_target):
                with manage_transaction(self.wabengine):
                    effective_target = self.wabengine.get_effective_target(
                        selected_target
                    )
                    Logger().info("Engine get_effective_target done (application)")
                    return effective_target
            else:
                Logger().info("Engine get_effective_target done (physical)")
                return [selected_target]

        except Exception:
            Logger().info(f"Engine get_effective_target failed: ((({traceback.format_exc()})))")
        return []

    # NOTE [RDP] unused
    def secondary_failed(self, reason, wabuser, ip_source, user, host) -> None:
        if self.failed_secondary_set:
            return
        self.failed_secondary_set = True
        if reason:
            self.session_diag = reason
        self.session_result = False
        Notify(self.wabengine, SECONDARY_CX_FAILED, {
            'user': wabuser,
            'ip': ip_source,
            'account': user,
            'device': host,
        })

    def get_app_params(self, selected_target: RightType, effective_target: RightType) -> Optional[AppParams]:
        # Logger().info("Engine get_app_params: "
        #               "{service_login=} {effective_target=}")
        if self.is_sharing_session(selected_target):
            _status, infos = self.check_target(selected_target)
            token = infos.get("shadow_token", {})
            app_params = AppParams("", None, token.get("shadow_id"))
            Logger().info("Engine get_app_params shadow done")
            return app_params
        try:
            with manage_transaction(self.wabengine):
                app_params = self.wabengine.get_app_params(
                    selected_target,
                    effective_target
                )
                Logger().info("Engine get_app_params done")
                return app_params
        except Exception:
            Logger().info(f"Engine get_app_params failed: ((({traceback.format_exc()})))")
        return None

    def get_primary_password(self, target_device: RightType) -> Optional[str]:
        Logger().debug("Engine get_primary_password ...")
        try:
            password = self.checkout.get_primary_password(target_device)
            Logger().debug("Engine get_primary_password done")
            return password
        except Exception:
            Logger().debug("Engine get_primary_password failed:"
                           f" ((({traceback.format_exc()})))")
        return None

    def get_account_infos(self, account_name: str,
                          domain_name: str, device_name: str) -> Optional[AccountInfos]:
        Logger().debug("Engine get_account_infos ...")
        try:
            return self.checkout.get_scenario_account_infos(
                account_name, domain_name, device_name
            )
        except Exception:
            Logger().debug("Engine get_account_infos failed:"
                           f" {traceback.format_exc()}")
        return None

    def get_account_infos_by_type(self, account_name: str, domain_name: str,
                                  device_name: str, with_ssh_key: bool = False,
                                  account_type: Optional[AccountType] = None) -> Optional[Dict[str, Any]]:
        try:
            return self.checkout.check_account_by_type(
                account_name=account_name,
                domain_name=domain_name,
                device_name=device_name,
                with_ssh_key=with_ssh_key,
                account_type=account_type
            )
        except Exception:
            Logger().debug("Engine get_account_infos_by_type failed:"
                           f" {traceback.format_exc()}")
        return None

    def get_target_passwords(self, target_device: RightType) -> List[str]:
        Logger().debug("Engine get_target_passwords ...")
        try:
            passwords = self.checkout.get_target_passwords(target_device)
        except Exception:
            Logger().debug("Engine get_target_passwords failed:"
                           f" ((({traceback.format_exc()})))")
            return []
        if self.is_sharing_session(target_device):
            return [
                password + ' ' + self.user_cn
                for password in passwords
            ]
        return passwords

    def get_target_password(self, target_device: RightType) -> str:
        passwords = self.get_target_passwords(target_device)
        return passwords[0] if passwords else ""

    def get_target_privkeys(self, target_device: RightType) -> List[Tuple[Any, Any, Any]]:
        Logger().debug("Engine get_target_privkeys ...")
        try:
            return self.checkout.get_target_privkeys(target_device)
        except Exception:
            Logger().debug("Engine get_target_privkey failed:"
                           f" ((({traceback.format_exc()})))")
        return []

    def release_target(self, target_device: RightType) -> bool:
        try:
            self.checkout.release_target(target_device)
        except Exception:
            Logger().debug("Engine release_target failed:"
                           f" ((({traceback.format_exc()})))")
        return True

    def release_account_by_type(self, account_name: str, domain_name: str,
                                device_name: str, account_type: Optional[AccountType] = None
                                ) -> bool:
        try:
            self.checkout.release_account_by_type(
                account_name, domain_name, device_name,
                account_type=account_type
            )
        except Exception:
            Logger().debug("Engine checkin_account_by_type failed:"
                           f" ({traceback.format_exc()})")
        return True

    def release_all_target(self) -> None:
        if self.checkout is not None:
            self.checkout.release_all()

    def start_session(self, auth, pid, effective_login=None, **kwargs
                      ) -> Tuple[Optional[str], Optional[int], str]:
        Logger().debug("**** CALL wabengine START SESSION ")
        error_msg = ""
        try:
            with manage_transaction(self.wabengine):
                self.session_id, self.start_time = self.wabengine.start_session(
                    auth,
                    pid=pid,
                    effective_login=effective_login,
                    **kwargs
                )
                self.failed_secondary_set = False
                if self.session_id is None:
                    return None, None, "SESSION_ID_IS_NONE"
                # Assume error status until proxy handle it
                self.set_session_status(diag='Connection aborted',
                                        result=False)
        except LicenseException as e:
            Logger().info("Engine start_session failed: License Exception")
            self.session_id, self.start_time, error_msg = \
                None, None, e.__class__.__name__
        except Exception as e:
            self.session_id, self.start_time, error_msg = \
                None, None, e.__class__.__name__
            Logger().info("Engine start_session failed:"
                          f" ((({traceback.format_exc()})))")
        Logger().debug("**** END wabengine START SESSION ")
        return self.session_id, self.start_time, error_msg

    # NOTE [RDP] calls to set_session_status() always initialize diag
    def start_session_ssh(self, target, target_user, hname, host, client_addr,
                          pid, subproto, kill_handler, effective_login=None,
                          **kwargs):
        """ Start session for new wabengine """
        self.hname = hname
        self.target_user = effective_login or target_user
        self.host = host

        if kill_handler:
            import signal
            signal.signal(signal.SIGUSR1, kill_handler)
        Logger().debug("**** CALL wabengine START SESSION ")

        error_msg = ""
        try:
            with manage_transaction(self.wabengine):
                self.session_id, self.start_time = self.wabengine.start_session(
                    target,
                    pid=pid,
                    subprotocol=subproto,
                    effective_login=effective_login,
                    target_host=self.host,
                    **kwargs
                )
        except LicenseException as e:
            Logger().info("Engine start_session failed: License exception")
            self.session_id, self.start_time, error_msg = (
                None, None, e.__class__.__name__)
        except Exception as e:
            self.session_id, self.start_time, error_msg = (
                None, None, e.__class__.__name__)
            Logger().info("Engine start_session failed:"
                          f" ((({traceback.format_exc()})))")
        Logger().debug("**** END wabengine START SESSION ")
        if self.session_id is None:
            return None, None, error_msg
        self.service = target['service_cn']
        is_critical = target['auth_is_critical']
        device_host = target['device_host']
        self.failed_secondary_set = False

        if not is_critical:
            return self.session_id, self.start_time, error_msg
        # Notify start
        # if subproto in ['SSH_X11_SESSION', 'SSH_SHELL_SESSION']:
        #     subproto = 'SSH'
        notif_data = {
            'protocol': subproto,
            'user': self.user_cn,
            'source': socket.getfqdn(client_addr),
            'ip_source': client_addr,
            'login': self.get_account_login(target),
            'device': hname,
            'ip': device_host,
            'time': time.ctime()
        }

        Notify(self.wabengine, CX_EQUIPMENT, notif_data)

        return self.session_id, self.start_time, error_msg

    def update_session_target(self, physical_target: RightType, **kwargs) -> None:
        """Update current session with target name.

        :param target physical_target: selected target
        :return: None
        """
        try:
            if self.session_id:
                account_name = physical_target['account_name']
                sep = '@' if physical_target['domain_cn'] else ''
                cn = physical_target['domain_cn']
                device = physical_target['device_cn']
                service = physical_target['service_cn']

                hosttarget = f"{account_name}{sep}{cn}@{device}:{service}"
                with manage_transaction(self.wabengine):
                    self.wabengine.update_session(self.session_id,
                                                  hosttarget=hosttarget,
                                                  **kwargs)
        except Exception:
            Logger().info("Engine update_session_target failed:"
                          f" ((({traceback.format_exc()})))")

    def update_session(self, **kwargs) -> None:
        """Update current session parameters to base.

        :return: None
        """
        try:
            if self.session_id:
                with manage_transaction(self.wabengine):
                    self.wabengine.update_session(self.session_id,
                                                  **kwargs)
        except Exception:
            Logger().info("Engine update_session failed:"
                          f" ((({traceback.format_exc()})))")

    def sharing_response(self, errcode: int, errmsg: str, token: Dict[str, Any], request_id: str) -> None:
        try:
            status = SHADOW_ACCEPTED if errcode == '0' else SHADOW_REJECTED
            with manage_transaction(self.wabengine):
                self.wabengine.make_session_sharing_response(
                    status=status, errmsg=errmsg,
                    token=token, request_id=request_id
                )
        except Exception:
            Logger().info("Engine sharing_response failed:"
                          f" ((({traceback.format_exc()})))")

    def stop_session(self, title: str = "End session") -> None:
        try:
            if self.session_id:
                # Logger().info(
                #     f"Engine stop_session: result='{self.session_result}', "
                #     f"diag='{self.session_diag}', title='{title}'"
                # )
                with manage_transaction(self.wabengine):
                    ret = self.wabengine.stop_session(
                        self.session_id,
                        result=self.session_result,
                        diag=self.session_diag,
                        title=title,
                        check=self.trace_hash
                    )
                    self.trace_hash = None

                if ret and IS_CLOUD_CONFIGURATION:
                    from wallixcloudbastion.utils import (
                        move_trace_immediately_as_process
                    )
                    move_trace_immediately_as_process(self.session_id)
        except SessionAlreadyStopped:
            pass
        except Exception:
            Logger().info("Engine stop_session failed:"
                          f" ((({traceback.format_exc()})))")
        Logger().debug("Engine stop session end")

    # NOTE [RDP] unused
    # RESTRICTIONS
    def get_all_restrictions(self, auth, proxytype) -> Tuple[Dict[str, List[str]], Dict[str, List[str]]]:
        if proxytype == "RDP":
            def matchproto(x):
                return x == "RDP"
        elif proxytype == "SSH":
            def matchproto(x):
                return x in {"SSH_SHELL_SESSION",
                             "SSH_REMOTE_COMMAND",
                             "SSH_SCP_UP",
                             "SSH_SCP_DOWN",
                             "SFTP_SESSION",
                             "RLOGIN", "TELNET"}
        else:
            return {}, {}
        try:
            with manage_transaction(self.wabengine):
                restrictions = self.wabengine.get_proxy_restrictions(auth)
            kill_patterns = {}
            notify_patterns = {}
            for restriction in restrictions:
                if not restriction.subprotocol:
                    Logger().error("No subprotocol in restriction!")
                    continue
                subproto = restriction.subprotocol.cn
                if matchproto(subproto):
                    # Logger().debug("adding restriction "
                    #                f"{restriction.action} {restriction.data} {subproto}"
                    # )
                    if restriction.action == 'kill':
                        kill_patterns.setdefault(subproto, []).append(restriction.data)
                    elif restriction.action == 'notify':
                        notify_patterns.setdefault(subproto, []).append(restriction.data)

            Logger().info(f"patterns_kill = {kill_patterns}")
            Logger().info(f"patterns_notify = {notify_patterns}")
        except Exception:
            kill_patterns = {}
            notify_patterns = {}
            Logger().info("Engine get_restrictions failed:"
                          f" ((({traceback.format_exc()})))")
        return (kill_patterns, notify_patterns)

    def get_restrictions(self, auth: RightType, proxytype: str) -> Tuple[Optional[str], Optional[str]]:
        if self.is_sharing_session(auth):
            return None, None
        if proxytype == "RDP":
            def matchproto(x):
                return x == "RDP"
            separator = "\x01"
        elif proxytype == "SSH":
            def matchproto(x):
                return x in {"SSH_SHELL_SESSION",
                             "SSH_REMOTE_COMMAND",
                             "SSH_SCP_UP",
                             "SSH_SCP_DOWN",
                             "SFTP_SESSION",
                             "RLOGIN", "TELNET"}
            separator = "|"
        else:
            return None, None
        try:
            with manage_transaction(self.wabengine):
                restrictions = self.wabengine.get_proxy_restrictions(auth)
            kill_patterns = []
            notify_patterns = []
            for restriction in restrictions:
                if not restriction.subprotocol:
                    Logger().error("No subprotocol in restriction!")
                    continue
                if matchproto(restriction.subprotocol.cn):
                    Logger().debug(f"adding restriction {restriction} "
                                   f"{restriction.data} {restriction.subprotocol.cn}")
                    if restriction.action == 'kill':
                        kill_patterns.append(restriction.data)
                    elif restriction.action == 'notify':
                        notify_patterns.append(restriction.data)

            self.pattern_kill = separator.join(kill_patterns)
            self.pattern_notify = separator.join(notify_patterns)
            Logger().info(f"pattern_kill = [{self.pattern_kill}]")
            Logger().info(f"pattern_notify = [{self.pattern_notify}]")
        except Exception:
            self.pattern_kill = None
            self.pattern_notify = None
            Logger().info("Engine get_restrictions failed:"
                          f" ((({traceback.format_exc()})))")
        return (self.pattern_kill, self.pattern_notify)

    # NOTE [RDP] unused
    # RESTRICTIONS: NOTIFIER METHODS
    def pattern_found_notify(self, action, regex_found, current_line: int):
        self.session_diag = f'Restriction pattern detected ({current_line})'
        data = {
            "regexp": regex_found,
            "string": current_line,
            "host": self.host,
            "user_login": self.user_cn,
            "user": self.target_user,
            "device": self.hname,
            "service": self.service,
            "action": action
        }
        Notify(self.wabengine, PATTERN_FOUND, data)
        Logger().info(
            f"{action}: The string '{current_line}' has been detected "
            "in the following SSH connection: "
            f"{self.target_user}@{self.hname}:{self.service}:{self.user_cn} ({self.host})\n"
        )
        if action.lower() == "kill":
            self.session_result = False

    # NOTE [RDP] unused
    def filesize_limit_notify(self, action, filesize, filename,
                              limit_filesize):
        restrictstr = f"file {filename} > {limit_filesize}"
        self.session_diag = f'Filesize restriction detected ({restrictstr})'
        data = {
            "regexp": f"filesize > {filesize}",
            "string": restrictstr,
            "host": self.host,
            "user_login": self.user_cn,
            "user": self.target_user,
            "device": self.hname,
            "service": self.service,
            "action": action
        }
        Notify(self.wabengine, PATTERN_FOUND, data)
        Logger().info(
            f"{action}: The restriction '{restrictstr}' has been detected "
            "in the following SSH connection: "
            f"{self.target_user}@{self.hname}:{self.service}:{self.user_cn} ({self.host})\n"
        )
        if action.lower() == "kill":
            self.session_result = False

    # NOTE [RDP] unused
    def globalsize_limit_notify(self, action, globalsize, limit_globalsize):
        self.session_diag = 'Filesize restriction detected'
        data = {
            "regexp": f"globalsize > {globalsize}",
            "string": f"globalsize > {limit_globalsize}",
            "host": self.host,
            "user_login": self.user_cn,
            "user": self.target_user,
            "device": self.hname,
            "service": self.service,
            "action": action
        }
        Notify(self.wabengine, PATTERN_FOUND, data)
        Logger().info(
            f"{action}: The restriction 'globalsize > {limit_globalsize}' has been detected "
            "in the following SSH connection: "
            f"{self.target_user}@{self.hname}:{self.service}:{self.user_cn} ({self.host})\n"
        )
        if action.lower() == "kill":
            self.session_result = False

    # NOTE [RDP] unused
    def start_tcpip_record(self, selected_target=None, filename=None):
        target = selected_target or self.target_right
        if not target:
            Logger().debug("start_record failed: missing target right")
            return False
        try:
            is_recorded = target['auth_is_recorded']
            if is_recorded:
                with manage_transaction(self.wabengine):
                    self.session_record = self.wabengine.get_trace_writer(
                        self.session_id,
                        filename=filename,
                        trace_type='pcap'
                    )
                self.session_record_type = "pcap"
                self.session_record.initialize()
        except Exception:
            Logger().info("Engine start_record failed")
            Logger().debug(f"Engine get_trace_writer failed: {traceback.format_exc()}")
            return False
        return True

    def write(self, data):
        if self.session_record:
            self.session_record.writeraw(data)

    # NOTE [RDP] unused
    def start_record(self, selected_target: Optional[RightType] = None, filename: Optional[str] = None) -> bool:
        target = selected_target or self.target_right
        if not target:
            Logger().debug("start_record failed: missing target right")
            return False
        try:
            is_recorded = target['auth_is_recorded']
            if is_recorded:
                with manage_transaction(self.wabengine):
                    self.session_record = self.wabengine.get_trace_writer(
                        self.session_id,
                        filename=filename,
                        trace_type='ttyrec'
                    )
                self.session_record_type = "ttyrec"
                self.session_record.initialize()
        except Exception:
            Logger().info("Engine start_record failed")
            Logger().debug(f"Engine get_trace_writer failed: {traceback.format_exc()}")
            return False
        return True

    # NOTE [RDP] unused
    def record(self, data) -> None:
        """ Factorized record method to be used if is_recorded == True """
        if self.session_record:
            self.session_record.writeframe(data)

    # NOTE [RDP] unused
    def stop_record(self) -> Optional[str]:
        if self.session_record:
            try:
                if self.session_record_type == "ttyrec":
                    # force last timestamp to match session duration
                    self.session_record.writeframe(b"")
                rec_hash = self.session_record.end()
                self.session_record = None
                return rec_hash
            except Exception as e:
                Logger().info(f"Stop record failed: {e}")
            self.session_record = None
        return None

    def write_trace(self, video_path: str) -> Tuple[bool, str]:
        try:
            _status, _error = True, "No error"
            if video_path:
                with manage_transaction(self.wabengine):
                    # Notify WabEngine with Trace file descriptor
                    trace = self.wabengine.get_trace_writer(
                        self.session_id,
                        filename=video_path,
                        trace_type="rdptrc"
                    )
                trace.initialize()
                trace.writeframe(f"{video_path}.mwrm".encode("utf8"))
                self.trace_hash = trace.end()
                self.session_record_type = "rdptrc"
        except Exception as e:
            Logger().info(f"Engine write_trace failed: {e}")
            _status, _error = False, "Exception"
        return _status, _error

    def read_session_parameters(self) -> Dict[str, Any]:
        with manage_transaction(self.wabengine):
            return self.wabengine.read_session_parameters(self.session_id)

    def check_target(self, target: RightType, request_ticket: Optional[Dict[str, Any]] = None
                     ) -> Tuple[str, Dict[str, Any]]:
        if self.checktarget_cache == (APPROVAL_ACCEPTED, target['target_uid']):
            # Logger().info("** CALL Check_target SKIPED**")
            return self.checktarget_cache[0], self.checktarget_infos_cache
        Logger().debug(f"** CALL Check_target ** ticket={request_ticket}")
        status, infos = self.checkout.check_target(target, request_ticket)
        Logger().debug("** END Check_target ** returns => "
                       f"status={status}, info fields={infos.keys()}")
        self.checktarget_cache = (status, target['target_uid'])
        self.checktarget_infos_cache = infos
        # Logger().info(f"returns => {status=}, {infos=}")
        deconnection_time = infos.get("deconnection_time")
        if deconnection_time:
            target['deconnection_time'] = deconnection_time
            # update deconnection_time in right
        return status, infos

    def check_effective_target(self, app_right: RightType, effective_target: RightType) -> bool:
        target_uid = effective_target['target_uid']
        for r in self.get_effective_target(app_right):
            if r['target_uid'] == target_uid:
                return True
        return False

    def get_application(self, selected_target: Optional[RightType] = None) -> Any:
        target = selected_target or self.target_right
        if not target:
            return None
        return target['application_cn']

    def get_target_protocols(self, selected_target: Optional[RightType] = None) -> Optional[ProtocolInfo]:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return None
        proto = target['service_protocol_cn']
        # subproto = [x.cn for x in target.resource.service.subprotocols]
        subproto = target['service_subprotocols']
        return ProtocolInfo(proto, subproto)

    def get_target_extra_info(self, selected_target: Optional[RightType] = None) -> Optional[ExtraInfo]:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return None
        is_recorded = target['auth_is_recorded']
        is_critical = target['auth_is_critical']
        has_approval = target['auth_has_approval']
        return ExtraInfo(is_recorded, is_critical, has_approval)

    def get_deconnection_time(self, selected_target: Optional[RightType] = None) -> Optional[str]:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return None
        if self.is_sharing_session(target):
            return '-'
        return target['deconnection_time']

    @staticmethod
    def get_bastion_timezone(time_ctx: TimeCtx) -> str:
        offset_seconds = time_ctx.altzone if (time_ctx.localtime().tm_isdst and time_ctx.daylight) else time_ctx.timezone
        offset_hours = abs(offset_seconds) // 3600
        offset_minutes = abs(offset_seconds) % 3600 // 60
        sign = '+' if offset_seconds <= 0 else '-'
        return f"UTC{sign}{offset_hours:02}:{offset_minutes:02}"

    # NOTE [RDP] unused
    def get_server_pubkey_options(self, selected_target: Optional[RightType] = None):
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return {}

        conn_policy_data = self.get_target_conn_options(target)
        return conn_policy_data.get('server_pubkey', {})

    def get_target_auth_methods(self, selected_target: Optional[RightType] = None) -> List[str]:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return []
        if self.is_sharing_session(target):
            return [PASSWORD_VAULT]
        try:
            # Logger().info("connectionpolicy")
            # Logger().info(f"{target.resource.service.connectionpolicy}")
            authmethods = target['connection_policy_methods']
        except Exception:
            Logger().error("Error: Connection policy has no methods field")
            authmethods = []
        return authmethods

    def get_target_conn_options(self, selected_target: Optional[RightType] = None) -> RightType:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return {}
        if self.is_sharing_session(target):
            return SHARING_CONN_POLICY
        try:
            # Logger().info("connectionpolicy")
            # Logger().info(f"{target.resource.service.connectionpolicy}")
            conn_opts = target['connection_policy_data']
        except Exception:
            Logger().error("Error: Connection policy has no data field")
            conn_opts = {}
        return conn_opts

    def get_target_conn_type(self, selected_target: Optional[RightType] = None) -> str:
        target = selected_target or self.target_right
        # TODO really possible?
        if not target:
            return 'RDP'
        try:
            conn_type = target.get('connection_policy_type',
                                   target.get('service_protocol_cn'))
        except Exception:
            Logger().error("Error: Connection policy has no data field")
            conn_type = "RDP"
        return conn_type

    def get_physical_target_info(self, physical_target: RightType) -> PhysicalTarget:
        if self.is_sharing_session(physical_target):
            _status, infos = self.check_target(physical_target)
            token = infos.get("shadow_token", {})
            return PhysicalTarget(
                device_host=(token.get('host_target_ip')
                             or physical_target['device_host']),
                account_login=(token.get('host_target_login')
                               or physical_target.get('account_login')),
                service_port=token.get('shadow_port'),
                device_id=physical_target.get('device_uid'),
                sharing_host=token.get('shadow_ip'),
            )
        port = physical_target['service_port']
        if isinstance(port, str):
            port = int(port)
        return PhysicalTarget(
            device_host=physical_target['device_host'],
            account_login=self.get_account_login(physical_target),
            service_port=port,
            device_id=physical_target['device_uid']
        )

    def get_target_login_info(self, selected_target: Optional[RightType] = None) -> Optional[LoginInfo]:
        target = selected_target or self.target_right
        if not target:
            return None
        if target['application_cn']:
            target_name = target['application_cn']
            device_host = None
        else:
            target_name = target['device_cn']
            device_host = target['device_host']

        account_login = self.get_account_login(target)
        account_name = target['account_name']
        domain_name = target['domain_cn']
        if domain_name == AM_IL_DOMAIN:
            domain_name = ""
        service_port = target['service_port']
        service_name = target['service_cn']
        auth_name = target['auth_cn']
        user_group_name = target.get('user_group_cn', "")
        target_group_name = target.get('target_group_cn', "")
        conn_opts = target['connection_policy_data']
        return LoginInfo(account_login=account_login,
                         account_name=account_name,
                         domain_name=domain_name,
                         target_name=target_name,
                         service_name=service_name,
                         auth_name=auth_name,
                         user_group_name=user_group_name,
                         target_group_name=target_group_name,
                         device_host=device_host,
                         service_port=service_port,
                         conn_opts=conn_opts)

    def get_account_login(self, right: RightType, check_in_creds: bool = True) -> str:
        login = right['account_login']
        domain = right.get('domain_name', '')
        if check_in_creds:
            login = (self.checkout.get_target_login(right) or login)
            domain = (self.checkout.get_target_domain(right) or domain)
        if not login and right['domain_cn'] == AM_IL_DOMAIN:
            # Interactive Login
            return login
        trule = right['connection_policy_data'].get(
            "general", {}
        ).get(
            "transformation_rule"
        )
        if (trule and '${LOGIN}' in trule):
            return trule.replace(
                '${LOGIN}', login
            ).replace(
                '${DOMAIN}', domain or ''
            )
        if not domain:
            return login
        return f"{login}@{domain}"

    def get_scenario_account(self, param, force_device) -> Dict[str, Any]:
        from .parsers import resolve_scenario_account
        return resolve_scenario_account(self, param, force_device)

    # NOTE [RDP] unused
    def get_crypto_methods(self):
        class crypto_methods:
            def __init__(self, proxy):
                self.proxy = proxy

            def get_trace_sign_key(self):
                return self.proxy.get_trace_sign_key()

            def get_trace_encryption_key(self, name, flag):
                return self.proxy.get_trace_encryption_key(name, flag)
        return crypto_methods(self.wabengine)

    def is_sharing_session(self, selected_target: Optional[RightType] = None) -> bool:
        target = selected_target or self.target_right
        return (
            target.get('is_shadow', False)
            or target.get('is_sharing', False)
            or target.get('is_sharing_type')
            or False
        )

    def get_sharing_session_type(self, selected_target: Optional[RightType] = None) -> str:
        target = selected_target or self.target_right
        sharing_type = target.get('is_sharing_type')
        if target.get('is_sharing', False):
            sharing_type = "SHARING"
        elif target.get('is_shadow', False):
            sharing_type = "SHADOWING"
        return sharing_type


class DisplayInfo:
    __slots__ = (
        "group",
        "host",
        "protocol",
        "service_login",
        "service_name",
        "subprotocols",
        "target_login",
        "target_name",
    )

    def __init__(self, target_login: str, target_name: str, service_name: str,
                 protocol: str, group: str, subproto: Protocols, host: str):
        self.target_login = target_login
        self.target_name = target_name
        self.service_name = service_name
        self.protocol = protocol
        self.group = group
        self.subprotocols = subproto or []
        self.service_login = f"{self.target_login}@{self.target_name}:{self.service_name}"
        self.host = host

    def get_target_tuple(self) -> Tuple[str, str, str, str]:
        return (self.target_login,
                self.target_name,
                self.service_name,
                self.group)


SHARING_CONN_POLICY = {
    'session': {
        'inactivity_timeout': 100000,
    },
    'session_probe': {
        'enable_session_probe': False,
    },
    'server_cert': {
        'server_cert_store': False,
        'server_cert_check': 3,
    },
    'session_log': {
        'keyboard_input_masking_level': 0,
    },
    'rdp': {
        'allow_tls_only_fallback': True,
        'allow_rdp_legacy_fallback': True,
    },
}
