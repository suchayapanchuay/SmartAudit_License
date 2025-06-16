# Copyright (c) 2021 WALLIX, SAS. All rights reserved.
#
# Licensed computer software. Property of WALLIX.

import os
import fcntl
import shlex
import binascii
import pexpect
import tempfile
from typing import AnyStr, Tuple, Optional, Dict, Union, Any
from subprocess import Popen, PIPE
from time import (
    monotonic as get_time,
    sleep,
)
try:
    import Cryptodome
except ImportError:
    import sys
    import Crypto
    sys.modules["Cryptodome"] = Crypto

from Cryptodome.PublicKey import RSA
from Cryptodome.Random import get_random_bytes

from wallix.logger import Logger
from wallixconst.misc import VOLATILE_FOLDER


class OpenSSLException(Exception):
    pass


SSH_KEYGEN = "/usr/bin/ssh-keygen"


def sshkeygen_add_passphrase_private(
        key: str,
        passphrase: Optional[str] = None
) -> str:
    """
    Transform an internal representation (openssh or pkcs1/pem) of a private key to an openssh
    or pkcs1/pem format with/without passphrase encryption
    :param key: a string representing the key in openssh or pkcs1/pem format
    :param passphrase: a string representing the passphrase used to cypher the
        exported key
    :return: a string representing the key in openssh or pkcs1/pem format

    pkcs1 format (the type - RSA, DSA - and the length are encoded in the ASN.1
    structure represented below by the base64 encoded DER:

    -----BEGIN DSA PRIVATE KEY-----
    MIIBugIBAAKBgQCBiEnMXvBFt6fXtPSpoMNNy4VK2a2RsPjBN9YSRxL+rAHnh6Z9
    VwccRi9Oa+4jhmU4POSEnzDqh52igs2S6YTt5QVPj7usiwYvQFYAmaLBKdOAWaGV
    9XoZ5BKcIoRkCK5lcRKHMksR+O1SmiEOv3+KhJUi0OKGD4vPkU5f9uLKFwIVAPGm
    uhM1NUIPFWRoR8YLWFTrjH+9AoGAE4BMlEaC9cjgsIMOhkfQkynVsCqDVsFTdlta
    xPfkFQb4ZIs3XTGnxTFiSRyzI4IwKD5R/JIzvdVXUONgp83ajRmDhzq1l8rF2x8p
    1T5yjZxQHeyCqWIpUj2ry+7/9+f3uKjmzQlSiNNbRATX+WBelMsT6FhZm6zwbtlk
    seAy1CQCgYAhR6lsnbShE6x/9c/UwOWA+DXcs0j0IwgN907BhwdjMt7gx2uo7EQh
    cDebNDnci5en9alQCmyKll6xlrOXLD+BOBTGc0mXN+dVQd1awjZ/aR15vuneg/A8
    VrmhrpcIr4EFBEueJlSUkG/xu2AhxxMv1jgSeWRiTG+Q6kihUEQ8lAIUTkUJJGFi
    Qn7Oy594TzrT89Y/qms=
    -----END DSA PRIVATE KEY-----
    """
    prk_fd, prk_path = tempfile.mkstemp(dir=VOLATILE_FOLDER)
    if not isinstance(key, bytes):
        key = key.encode('utf-8')
    if passphrase is None:
        passphrase = ""
    os.write(prk_fd, key)
    os.close(prk_fd)
    command = f"{SSH_KEYGEN} -f {prk_path} -p"
    new_passphrase = ".*Enter new passphrase.*"
    confirm_passphrase = ".*Enter same passphrase again:"
    ssh_keygen = pexpect.spawn(command)
    i = ssh_keygen.expect([new_passphrase, pexpect.EOF])
    if i == 0:
        ssh_keygen.sendline(passphrase)
    else:
        msg = "Error setting the passphrase for the exported private key"
        os.remove(prk_path)
        raise OpenSSLException(msg)
    i = ssh_keygen.expect([confirm_passphrase, pexpect.EOF])
    if i == 0:
        ssh_keygen.sendline(passphrase)
    else:
        msg = "Error setting the passphrase for the exported private key"
        os.remove(prk_path)
        raise OpenSSLException(msg)
    ssh_keygen.expect([pexpect.EOF])
    if ssh_keygen.isalive():
        ssh_keygen.wait()
    with open(prk_path, 'r') as f:
        pem_key = f.read()
    os.remove(prk_path)
    return pem_key


RANDOM_NAME_SIZE = 10
CONNECTION_TIMEOUT = 5
SSHPASS_COPYABLE_VAR_ENV = ('LANG', 'PATH')


def crypto_random_str_by_bytes_size(bytes_size: int) -> str:
    return binascii.hexlify(get_random_bytes(bytes_size)).decode("ascii")


# subprocess popen
def set_non_blocking_fd(fd) -> None:
    """
    Set the file description of the given file descriptor to non-blocking.
    """
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    flags = flags | os.O_NONBLOCK
    fcntl.fcntl(fd, fcntl.F_SETFL, flags)


def popen_command(command: str, env: Dict[str, str]) -> Popen:
    args = shlex.split(command)
    process = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE,
                    universal_newlines=True, bufsize=1, env=env)
    set_non_blocking_fd(process.stdout)
    set_non_blocking_fd(process.stderr)
    return process


def popen_sshpass_ssh(command: str, ssh_password: str, ssh_private_key_passphrase: str) -> Popen:
    env = {k: os.environ[k] for k in SSHPASS_COPYABLE_VAR_ENV if k in os.environ}
    if ssh_private_key_passphrase:
        command = f"sshpass -P passphrase -e {command}"
        env["SSHPASS"] = ssh_private_key_passphrase
    else:
        command = f"sshpass -e {command}"
        env["SSHPASS"] = ssh_password
    return popen_command(command, env)


# p = Popen("./a.out", stdin = PIPE, stdout = PIPE, stderr = PIPE, bufsize = 1)
# setNonBlocking(p.stdout)
# setNonBlocking(p.stderr)


def pexpect_prompt_ssh(command: str, ssh_password: str, ssh_private_key_passphrase: str):
    px = pexpect.spawn(command)
    if ssh_private_key_passphrase:
        px.expect("passphrase")
    else:
        px.expect("assword:")
    px.sendline(ssh_private_key_passphrase or ssh_password)
    return px


def expect_connection_ready(file_path: str) -> bool:
    start_time = get_time()
    while not os.path.exists(file_path):
        sleep(0.5)
        if get_time() - start_time > CONNECTION_TIMEOUT:
            return False
    return True


def remove_file(file_path: str) -> None:
    try:
        os.remove(file_path)
    except OSError:
        pass


def add_passphrase_to_private_key(private_key: AnyStr, passphrase: str) -> str:
    try:
        in_key = RSA.importKey(private_key)
        return in_key.exportKey(passphrase=passphrase).decode("utf-8")
    except Exception:
        return sshkeygen_add_passphrase_private(private_key, passphrase)


class TunnelingProcessInterface:
    sock_path: str

    def start(self) -> bool:
        return False

    def pre_connect(self) -> bool:
        return True

    def post_connect(self) -> bool:
        return True

    def stop(self) -> None:
        return


class TunnelingProcessSSH(TunnelingProcessInterface):
    process: Optional[Any]

    def __init__(self,
                 target_host: str,
                 vnc_port: Union[int, str],
                 ssh_port: Union[int, str],
                 ssh_login: str,
                 ssh_password: str,
                 ssh_key: Optional[Tuple[AnyStr, Any, str]],
                 sock_path_dir: str) -> None:
        self.process = None
        self.sock_path = ''
        self.target_host = target_host
        self.vnc_port = vnc_port
        self.ssh_port = ssh_port
        self.ssh_login = ssh_login
        self.ssh_password = ssh_password
        self.sock_path_dir = sock_path_dir
        self._use_pexpect = False

        self.ssh_key_passphrase = ""
        self.ssh_key_private_key_filename = ""
        self.ssh_key_certificate_filename = ""

        if ssh_key:
            private_key, _, certificate = ssh_key

            passphrase = crypto_random_str_by_bytes_size(32)

            private_key_filename = (
                f"{VOLATILE_FOLDER}/{crypto_random_str_by_bytes_size(RANDOM_NAME_SIZE)}"
            )
            certificate_filename = private_key_filename + '.pub'

            try:
                with open(private_key_filename, 'w') as f:
                    os.chmod(private_key_filename, 0o400)
                    out_key = add_passphrase_to_private_key(
                        private_key=private_key,
                        passphrase=passphrase
                    )
                    f.write(out_key)

                    if certificate:
                        with open(certificate_filename, 'w') as f2:
                            os.chmod(certificate_filename, 0o400)
                            f2.write(certificate)

                            self.ssh_key_certificate_filename = certificate_filename

                    self.ssh_key_passphrase = passphrase
                    self.ssh_key_private_key_filename = private_key_filename
            except Exception:
                os.remove(private_key_filename)
                if certificate:
                    os.remove(certificate_filename)

                raise

    def _generate_sock_path(self) -> None:
        assert not self.sock_path
        pid = os.getpid()
        random_text = crypto_random_str_by_bytes_size(RANDOM_NAME_SIZE)
        generated_name = f"vnc-{pid}-{random_text}.sock"
        self.sock_path = os.path.join(self.sock_path_dir, generated_name)

    def start(self) -> bool:
        self._generate_sock_path()
        self.process = ssh_tunneling_vnc(
            self.sock_path, self.target_host, self.vnc_port,
            self.ssh_port, self.ssh_login, self.ssh_password,
            self.ssh_key_private_key_filename, self.ssh_key_passphrase,
            use_pexpect=self._use_pexpect,
        )

        if self.process is not None:
            return True

        self._remove_ssh_key_files()

        return False

    def pre_connect(self) -> bool:
        try:
            if not self.sock_path:
                return False
            return expect_connection_ready(self.sock_path)
        finally:
            self._remove_ssh_key_files()

    def _remove_socket_file(self) -> None:
        if self.sock_path:
            remove_file(self.sock_path)
            self.sock_path = ''

    def _remove_ssh_key_files(self) -> None:
        if self.ssh_key_private_key_filename:
            os.remove(self.ssh_key_private_key_filename)

        if self.ssh_key_certificate_filename:
            os.remove(self.ssh_key_certificate_filename)

        self.ssh_key_passphrase = ""
        self.ssh_key_private_key_filename = ""
        self.ssh_key_certificate_filename = ""

    def post_connect(self) -> bool:
        self._remove_socket_file()
        return True

    def stop(self) -> None:
        self._remove_socket_file()
        if self.process:
            self.process.terminate()
            try:
                self.process.wait()
            except Exception:
                self.process.kill(9)
            # Logger().debug(self.process.returncode)
            self.process = None


class TunnelingProcessPEXPECTSSH(TunnelingProcessSSH):
    def __init__(self, **kwargs):
        super().__init__(self, **kwargs)
        self._use_pexpect = True


class TunnelingProcessPXSSH(TunnelingProcessSSH):
    def start(self) -> bool:
        self._generate_sock_path()
        self.process = pxssh_ssh_tunneling_vnc(
            self.sock_path, self.target_host, self.vnc_port,
            self.ssh_port, self.ssh_login, self.ssh_password,
            self.ssh_key_private_key_filename, self.ssh_key_passphrase
        )

        if self.process is not None:
            return True

        TunnelingProcessSSH.pre_connect(self)

        return False

    def pre_connect(self) -> bool:
        # pxssh is blocking on connect
        TunnelingProcessSSH.pre_connect(self)
        return True

    def stop(self) -> None:
        self._remove_socket_file()
        if self.process:
            self.process.logout()
            self.process.terminate()
            try:
                self.process.wait()
            except Exception:
                # import traceback
                # print(traceback.format_exc())
                self.process.kill()
            # Logger().debug(self.process)
            self.process = None


def ssh_tunneling_vnc(local_usocket_name: str,
                      target_host: str,
                      vnc_port: Union[int, str],
                      ssh_port: Union[int, str],
                      ssh_login: str,
                      ssh_password: str,
                      ssh_private_key_filename: str,
                      ssh_private_key_passphrase: str,
                      use_pexpect: bool = False) -> Optional[Any]:
    """
    local_usocket_name :         must be absolute path
    target_host :                ssh and vnc host
    vnc_port :                   vnc port
    ssh_port :                   ssh port
    ssh_login :                  ssh login
    ssh_password :               ssh password
    ssh_private_key_filename :   ssh private key filename
    ssh_private_key_passphrase : ssh private key passphrase
    """
    ssh_opts = (
        "-o UserKnownHostsFile=/dev/null "
        "-o StrictHostKeyChecking=no "
        "-N "
        f"-p {ssh_port}"
    )

    use_private_key = bool(ssh_private_key_filename and ssh_private_key_passphrase)
    if use_private_key:
        ssh_opts += (
            f" -i {ssh_private_key_filename} "
            "-o PubkeyAcceptedKeyTypes=+ssh-dss"
        )

    tunneling_command = (
        f"ssh {ssh_opts} -N "
        f"-L {local_usocket_name}:127.0.0.1:{vnc_port} "
        f"-l {ssh_login} {target_host}"
    )
    remove_file(local_usocket_name)
    Logger().debug(f"ssh_tunneling_vnc {tunneling_command}")
    try:
        process_fn = pexpect_prompt_ssh if use_pexpect else popen_sshpass_ssh
        process = process_fn(
            tunneling_command,
            ssh_password,
            ssh_private_key_passphrase if use_private_key else ""
        )
    except Exception as e:
        Logger().info(f"Tunneling with "
                      f"{'Pexpect' if use_pexpect else 'Popen'} "
                      f"Error {e}")
        process = None
    return process


def pxssh_ssh_tunneling_vnc(local_usocket_name: str,
                            target_host: str,
                            vnc_port: Union[int, str],
                            ssh_port: Union[int, str],
                            ssh_login: str,
                            ssh_password: str,
                            ssh_private_key_filename: str,
                            ssh_private_key_passphrase: str) -> Optional[Any]:
    """
    local_usocket_name :         must be absolute path
    target_host :                ssh and vnc host
    vnc_port :                   vnc port
    ssh_port :                   ssh port
    ssh_login :                  ssh login
    ssh_password :               ssh password
    ssh_private_key_filename :   ssh private key filename
    ssh_private_key_passphrase : ssh private key passphrase
    """
    from pexpect import pxssh
    try:
        p = pxssh.pxssh(
            ignore_sighup=False,
            options={
                "StrictHostKeyChecking": "no",
                "UserKnownHostsFile": "/dev/null",
                "PubkeyAcceptedKeyTypes": "+ssh-dss"
            }
        )
        use_private_key = bool(ssh_private_key_filename and ssh_private_key_passphrase)
        if not use_private_key:
            p.force_password = True
        remove_file(local_usocket_name)
        p.login(
            server=target_host,
            username=ssh_login,
            ssh_key=ssh_private_key_filename if use_private_key else None,
            password=ssh_private_key_passphrase if use_private_key else ssh_password,
            port=ssh_port,
            ssh_tunnels={
                'local': [f'{local_usocket_name}:127.0.0.1:{vnc_port}']
            }
        )
        return p
    except Exception as e:
        Logger().info(f"Tunneling with PXSSH Error {e}")
        return None


def check_tunneling(engine,
                    opts: Dict[str, Any],
                    target_host: str,
                    target_port: Union[int, str],
                    sock_path_dir: str) -> TunnelingProcessInterface:
    ssh_port = opts.get("ssh_port")
    if opts.get("tunneling_credential_source") == "static_login":
        ssh_login = opts.get("ssh_login")
        ssh_password = opts.get("ssh_password")
        ssh_key = None
    else:
        acc_infos = engine.get_scenario_account(param=opts.get("scenario_account_name"),
                                                force_device=True)
        ssh_login = acc_infos.get('login')
        ssh_password = acc_infos.get('password')
        ssh_key = acc_infos.get('ssh_key')

    tunneling_type = opts.get("tunneling_type", "pxssh")
    if tunneling_type == "pexpect":
        tunneling_class = TunnelingProcessPEXPECTSSH
    elif tunneling_type == "popen":
        tunneling_class = TunnelingProcessSSH  # type: ignore
    else:
        tunneling_class = TunnelingProcessPXSSH  # type: ignore

    return tunneling_class(
        target_host=target_host,
        vnc_port=target_port,
        ssh_port=ssh_port,
        ssh_login=ssh_login,
        ssh_password=ssh_password or '',
        ssh_key=ssh_key,
        sock_path_dir=sock_path_dir
    )
