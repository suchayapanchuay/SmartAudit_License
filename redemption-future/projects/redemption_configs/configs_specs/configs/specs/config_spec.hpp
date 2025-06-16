/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2016
*   Author(s): Jonathan Poelen
*/

#pragma once

#include "configs/attributes/spec.hpp"
#include "configs/enumeration.hpp"

#include "configs/autogen/enums.hpp"
#include "configs/type_name.hpp"

#include "configs/generators/config.hpp"

#include "include/debug_verbose_description.hpp"

#include "keyboard/keylayouts.hpp"
#include "utils/theme.hpp"
#include "utils/colors.hpp"
#include "utils/file_permissions.hpp"
#include "utils/screen_resolution.hpp"

#include <chrono>
#include <vector>
#include <string>


namespace cfg_specs {

#ifdef IN_IDE_PARSER
    constexpr char const* CONFIG_DESC_AUTH = "";
    constexpr char const* CONFIG_DESC_FRONT = "";
    constexpr char const* CONFIG_DESC_RDP = "";
    constexpr char const* CONFIG_DESC_VNC = "";
    constexpr char const* CONFIG_DESC_SCK = "";
    constexpr char const* CONFIG_DESC_CAPTURE = "";
    constexpr char const* CONFIG_DESC_SESSION = "";
    constexpr char const* CONFIG_DESC_MOD_INTERNAL = "";
    constexpr char const* CONFIG_DESC_COMPRESSION = "";
    constexpr char const* CONFIG_DESC_CACHE = "";
    constexpr char const* CONFIG_DESC_OCR = "";
#endif
inline void config_spec_definition(cfg_generators::GeneratorConfigWrapper& _, type_enumerations const& tenums)
{

using namespace cfg_desc;

// force ordering section
_.set_sections({
    "general",

    "globals",

    "client",

    "all_target_mod",
    "remote_program",
    "mod_rdp",
    "protocol",
    "session_probe",
    "server_cert",
    "mod_vnc",

    "session_log",
    "ocr",
    "capture",
    "audit",

    "file_verification",
    "file_storage",
    "icap_server_down",
    "icap_server_up",

    "crypto",
    "websocket",

    "vnc_over_ssh",

    "context",

    "internal_mod",
    "mod_replay",
    "translation",
    "theme",

    "debug",
});

cfg_generators::EnumAsString enum_as_string{tenums};
cfg_generators::ValueFromEnum from_enum{tenums};
using cfg_generators::value;
using cfg_generators::rdp_all_policy_value;
using cfg_generators::rdp_general_policy_value;
using cfg_generators::rdp_sogisces_1_3_2030_policy_value;
using cfg_generators::vnc_policy_value;
using cfg_generators::MemberInfo;
using cfg_generators::Description;
using spec::proxy_to_acl;
using spec::acl_to_proxy;
using spec::acl_rw;
using spec::no_acl;
using spec::global_spec;
using spec::ini_only;
using spec::connpolicy;
using spec::acl_connpolicy;

auto loggable = Loggable::Yes;
auto non_loggable = Loggable::No;
auto loggable_when_contains_password_string = Loggable::OnlyWhenContainsPasswordString;

auto reset_back_to_selector = ResetBackToSelector::Yes;
auto no_reset_back_to_selector = ResetBackToSelector::No;

auto vnc = DestSpecFile::vnc;
auto rdp_general = DestSpecFile::rdp;
auto rdp_sogisces_1_3_2030 = DestSpecFile::rdp_sogisces_1_3_2030;
auto rdp = rdp_general | rdp_sogisces_1_3_2030;


// updated by acl
constexpr auto default_key =
    "\x00\x01\x02\x03\x04\x05\x06\x07"
    "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x14\x15\x16\x17"
    "\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"_sized_av
;

std::string_view disabled_orders_desc =
    "This option should only be used if the target server or client is showing graphical issues.\n"
    "In general, disabling RDP orders has a negative impact on performance.\n"
    "\n"
    "Drawing orders that can be disabled:\n"
    "   0: DstBlt\n"
    "   1: PatBlt\n"
    "   2: ScrBlt\n"
    "   3: MemBlt\n"
    "   4: Mem3Blt\n"
    "   9: LineTo\n"
    "  15: MultiDstBlt\n"
    "  16: MultiPatBlt\n"
    "  17: MultiScrBlt\n"
    "  18: MultiOpaqueRect\n"
    "  22: Polyline\n"
    "  25: EllipseSC\n"
    "  27: GlyphIndex"
;

std::string_view tls_1_3_ciphersuites_desc =
    "Configure the available TLSv1.3 ciphersuites.\n"
    "Empty to apply system-wide configuration.\n"
    "The format used is described in the third paragraph of this page: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_ciphersuites.html#DESCRIPTION"
;

std::string_view tls_key_exchange_groups =
    "Configure the supported key exchange groups.\n"
    "Empty to apply system-wide configuration.\n"
    "The format used is described in this page: https://www.openssl.org/docs/man3.2/man3/SSL_CONF_cmd.html#groups-groups"
;

std::string_view tls_signature_algorithms_desc =
    "Configure the supported server signature algorithms.\n"
    "Empty to apply system-wide configuration.\n"
    "The format should be a colon separated list of signature algorithms in order of decreasing preference of the form algorithm+hash or signature_scheme.\n"
    "algorithm is one of RSA, RSA-PSS or ECDSA.\n"
    "hash is one of SHA224, SHA256, SHA384 or SHA512.\n"
    "signature_scheme is one of the signature schemes defined in TLSv1.3 (rfc8446#section-4.2.3), specified using the IETF name, e.g., ecdsa_secp384r1_sha384 or rsa_pss_rsae_sha256."
;

std::string_view tls_signature_algorithms_server_extra_desc =
    "\nThis list needs at least one signature algorithm compatible with the RDP Proxy certificate.\n"
;

_.display_name_word_replacement_table = {
    {"rdp", "RDP"},
    {"vnc", "VNC"},
    {"ssh", "SSH"},

    {"tls", "TLS"},
    {"ssl", "SSL"},
    {"tcp", "TCP"},
    {"ip", "IP"},
    {"ipv4", "IPv4"},
    {"ipv6", "IPv6"},

    {"websocket", "WebSocket"},
    {"kerberos", "Kerberos"},
    {"ntlm", "NTLM"},
    {"nla", "NLA"},

    {"remoteapp", "RemoteApp"},
    {"remotefx", "RemoteFX"},
    {"rdpdr", "RDPDR"},

    {"appdriver", "Application Driver"},
    {"session_probe", "Session Probe"},
    {"bestsafe", "BestSafe"},
    {"am", "AM"},
    {"wabam", "AM"},

    {"osd", "OSD"},

    {"ios", "iOS"},
    {"macos", "MacOS"},
    {"unix", "Unix"},
    {"linux", "Linux"},

    {"redis", "Redis"},
    {"freerdp", "FreeRDP"},
    {"ffmpeg", "FFmpeg"},

    {"cpu", "CPU"},
    {"ui", "UI"},
};

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wc99-designator")

_.section("general", [&]
{
    _.member(MemberInfo{
        .name = "transformation_rule",
        .value = value<std::string>(),
        .spec = spec::acl_connpolicy(rdp | vnc),
        .desc =
            "Secondary login Transformation rule\n"
            "${LOGIN} will be replaced by login\n"
            "${DOMAIN} (optional) will be replaced by domain if it exists.\n"
            "Empty value means no transformation rule."
    });

    _.member(MemberInfo{
        .name = "vault_transformation_rule",
        .value = value<std::string>(),
        .spec = spec::acl_connpolicy(rdp | vnc),
        .desc =
            "Account Mapping password retriever\n"
            "Transformation to apply to find the correct account.\n"
            "${USER} will be replaced by the user's login.\n"
            "${DOMAIN} will be replaced by the user's domain (in case of LDAP mapping).\n"
            "${USER_DOMAIN} will be replaced by the user's login + \"@\" + user's domain (or just user's login if there's no domain).\n"
            "${GROUP} will be replaced by the authorization's user group.\n"
            "${DEVICE} will be replaced by the device's name.\n"
            "A regular expression is allowed to transform a variable, with the syntax: ${USER:/regex/replacement}, groups can be captured with parentheses and used with \\1, \\2, ...\n"
            "For example to replace leading \"A\" by \"B\" in the username: ${USER:/^A/B}\n"
            "Empty value means no transformation rule.\n"
    });
});

_.section("globals", [&]
{
    _.member(MemberInfo{
        .name = "front_connection_time",
        .value = value<std::chrono::milliseconds>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
        .desc = "From incoming connection to \"up_and_running\" state.",
    });

    _.member(MemberInfo{
        .name = "target_connection_time",
        .value = value<std::chrono::milliseconds>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
        .desc = "From Module rdp creation to \"up_and_running\" state.",
    });


    _.member(MemberInfo{
        .name = names{
            .all = "auth_user",
            .acl = "login",
        },
        .value = value<std::string>(),
        .spec = acl_rw(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "host",
            .acl = "ip_client",
        },
        .value = value<std::string>(),
        .spec = acl_rw(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "target",
            .acl = "ip_target",
        },
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "target_device",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "device_id",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "primary_user_id",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "target_user",
            .acl = "target_login",
        },
        .value = value<std::string>(),
        .spec = acl_rw(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_application",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_application_account",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_application_password",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "port",
        .value = value<types::unsigned_>(3389),
        .spec = global_spec(no_acl, spec::advanced | spec::iptables | spec::logged),
        .desc =
            "Port of RDP Proxy service.\n\n"
            "Changing the port number will restart the service and disconnect active sessions. It will also block WALLIX Access Manager connections unless its RDP port is configured.\n"
            "Choose a port that is not already in use. Otherwise, the service will not run.\n"
    });

    _.member(MemberInfo{
        .name = "authfile",
        .value = value<std::string>(CPP_EXPR(REDEMPTION_CONFIG_AUTHFILE)),
        .spec = ini_only(no_acl),
        .desc = "Socket path or socket address of passthrough / ACL.",
    });

    _.member(MemberInfo{
        .name = "handshake_timeout",
        .value = value<std::chrono::seconds>(10),
        .spec = global_spec(no_acl),
        .desc = "Timeout during RDP connection initialization.\n"
        "Increase this value if the connection between workstations and Bastion is slow.",
    });

    _.member(MemberInfo{
        .name = "base_inactivity_timeout",
        .value = value<std::chrono::seconds>(900),
        .spec = global_spec(no_acl),
        .desc =
            "No automatic disconnection due to inactivity, timer is set on primary authentication.\n"
            "If the value is between 1 and 30, then 30 is used.\n"
            "If the value is set to 0, then inactivity timeout value is unlimited.",
    });

    _.member(MemberInfo{
        .name = "inactivity_timeout",
        .connpolicy_section = "session",
        .value = value<std::chrono::seconds>(),
        .spec = connpolicy(rdp | vnc, loggable),
        .desc =
            "No automatic disconnection due to inactivity, timer is set on target session.\n"
            "If the value is between 1 and 30, then 30 is used.\n"
            "If the value is set to 0, then the value set in :REF:[globals]:base_inactivity_timeout is used."
    });

    _.member(MemberInfo{
        .name = "keepalive_grace_delay",
        .value = value<std::chrono::seconds>(30),
        .spec = ini_only(no_acl),
        .desc = "Internal keepalive between ACL and RDP proxy.",
    });

    _.member(MemberInfo{
        .name = "authentication_timeout",
        .value = value<std::chrono::seconds>(120),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Specifies how long the RDP proxy login screen should be displayed before the client window closes (use 0 to deactivate).",
    });

    _.member(MemberInfo{
        .name = "trace_type",
        .value = from_enum(TraceType::localfile_hashed),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "listen_address",
        .value = value<types::ip_string>("0.0.0.0"),
        .spec = ini_only(no_acl),
        .desc = "Specify bind address.",
    });

    _.member(MemberInfo{
        .name = "enable_transparent_mode",
        .value = value(false),
        .spec = global_spec(no_acl, spec::iptables),
        .desc = "Transparent mode allows network traffic interception for a target, even when users directly specify a target's address instead of a proxy address.",
    });

    _.member(MemberInfo{
        .name = "certificate_password",
        .value = value<types::fixed_string<254>>("inquisition"),
        .spec = ini_only(no_acl),
        .desc = "Proxy certificate password.",
    });

    _.member(MemberInfo{
        .name = "is_rec",
        .value = value(false),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    // TODO remove ?
    _.member(MemberInfo{
        .name = "enable_bitmap_update",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc = "Support of Bitmap Update.",
    });

    _.member(MemberInfo{
        .name = "enable_end_time_warning_osd",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Displays a reminder box at the top of the session when a session has a time limit (timeframe or approval).\n"
            "Reminders appear at 30 minutes, 10 minutes, 5 minutes, and 1 minute before the session ends."
    });

    _.member(MemberInfo{
        .name = "enable_osd_display_remote_target",
        .value = value(true),
        .spec = global_spec(acl_to_proxy(no_reset_back_to_selector, loggable), spec::advanced),
        .desc = "Allows showing the target device name with F12 during the session.",
    });

    _.member(MemberInfo{
        .name = "show_target_user_in_f12_message",
        .value = value(false),
        .spec = global_spec(no_acl),
        .desc =
            "Displays the target username in the session when F12 is pressed.\n"
            "This option needs :REF:[globals]:enable_osd_display_remote_target."
    });


    _.member(MemberInfo{
        .name = "enable_wab_integration",
        .value = value<bool>(CPP_EXPR(REDEMPTION_CONFIG_ENABLE_WAB_INTEGRATION)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "large_pointer_support",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc =
            "Enable support for 96x96 size pointers.\n"
            "⚠ If this option is disabled and the application does not support smaller pointers, the pointer may remain stuck on the last active pointer. For example, the resize window pointer would remain visible rather than change to a 'normal' pointer.",
    });

    _.member(MemberInfo{
        .name = "mod_recv_timeout",
        .value = value<types::range<std::chrono::milliseconds, 100, 10000>>(1000),
        .spec = ini_only(no_acl),
    });

    // TODO unused
    _.member(MemberInfo{
        .name = "experimental_enable_serializer_data_block_size_limit",
        .value = value(false),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "rdp_keepalive_connection_interval",
        .value = value<std::chrono::milliseconds>(),
        .spec = global_spec(no_acl),
        .desc =
            "Prevent Remote Desktop session timeouts due to idle TCP sessions by periodically sending keepalive packets to the client.\n"
            "Set to 0 to disable this feature."
    });

    _.member(MemberInfo{
        .name = "enable_ipv6",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "⚠ Manually restart service redemption to take changes into account.\n\n"
            "Enable primary connection on IPv6."
    });

    _.member(MemberInfo{
        .name = "minimal_memory_available_before_connection_silently_closed",
        .value = value<types::mebibytes<types::u64>>(100),
        .spec = ini_only(no_acl),
        .desc = "0 for disabled.",
    });
});

_.section("session_log", [&]
{
    _.member(MemberInfo{
        .name = "enable_session_log_file",
        .value = value(true),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc =
            "Saves session logs to a .log file.\n"
            "The format is a date followed by one or more key=\"value\" separated by a space on the same line.",
    });

    _.member(MemberInfo{
        .name = "enable_syslog_format",
        .value = from_enum(SessionLogFormat::SIEM),
        .spec = ini_only(no_acl),
        .desc = "Writes session logs to syslog and define its format.",
    });

    _.member(MemberInfo{
        .name = "keyboard_input_masking_level",
        .value = from_enum(KeyboardInputMaskingLevel::password_and_unidentified,
            // see capture::disable_keyboard_log
            vnc_policy_value(KeyboardInputMaskingLevel::unmasked).always()),
        .spec = connpolicy(rdp | vnc, loggable),
        .desc = "Classification of input data is performed using Session Probe.\n"
        "Without Session Probe, all the texts entered are considered unidentified.",
    });
});

_.section("client", [&]
{
    _.member(MemberInfo{
        .name = "ignore_logon_password",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "If true, ignore the password provided by the RDP client. The user needs to manually log in.",
    });

    _.member(MemberInfo{
        .name = "allow_using_multiple_monitors",
        .value = value(true),
        .spec = global_spec(no_acl),
        .tags = Tag::Compatibility,
        .desc = "Sends the client screen count to the target server. Not supported for VNC targets.\n"
        "Uncheck to disable multiple monitors.",
    });

    // TODO should be enabled by default ?
    _.member(MemberInfo{
        .name = "allow_scale_factor",
        .value = value(false),
        .spec = global_spec(no_acl),
        .tags = Tag::Compatibility,
        .desc =
            "Sends Scale & Layout configuration to the target server.\n"
            "On Windows 11, this corresponds to the options \"Scale\", \"Display Resolution\" and \"Display Orientation\" of Settings > System > Display.\n"
            "⚠ Title bar detection via OCR will no longer work.\n"
    });

    // TODO remove ?
    _.member(MemberInfo{
        .name = "unicode_keyboard_event_support",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc =
            "Allows the client to use unicode characters.\n"
            "This is useful for displaying characters that are not available on the keyboard layout used, such as some special characters or emojis."
    });


    _.member(MemberInfo{
        .name = "encryption_level",
        .value = enum_as_string(RdpSecurityEncryptionLevel::high),
        .spec = ini_only(no_acl),
        .desc = "Legacy encryption when External Security Protocol (TLS, CredSSP, etc) is disabled."
    });

    _.member(MemberInfo{
        .name = "tls_fallback_legacy",
        .value = value(false),
        .spec = global_spec(no_acl),
        .desc = "Fallback to RDP Legacy Encryption if the client does not support TLS.\n"
        "⚠ Enabling this option is a security risk.",
    });

    _.member(MemberInfo{
        .name = "tls_support",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc = "Enable TLS between client and proxy.\n",
    });

    _.member(MemberInfo{
        .name = "tls_min_level",
        .value = value<types::u32>(2),
        .spec = global_spec(no_acl),
        .desc = "Minimal incoming TLS level: 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3\n"
        "⚠ Lower this value only for compatibility reasons.",
    });

    _.member(MemberInfo{
        .name = "tls_max_level",
        .value = value<types::u32>(0),
        .spec = global_spec(no_acl),
        .desc = "Maximal incoming TLS level: 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3\n"
        "⚠ Change this value only for compatibility reasons.",
    });

    // since openssl 2, the security level is 2
    // "DEFAULT@SECLEVEL=1" (<= 1.1)
    // "DEFAULT@SECLEVEL=2" (>= 2)
    _.member(MemberInfo{
        .name = "ssl_cipher_list",
        .value = value<std::string>("HIGH:!ADH:!3DES:!SHA"),
        .spec = global_spec(no_acl),
        .desc =
            "[Not configured]: Compatible with more RDP clients (less secure)\n"
            "HIGH:!ADH:!3DES: Compatible only with MS Windows 7 client or more recent (moderately secure)\n"
            "HIGH:!ADH:!3DES:!SHA: Compatible only with MS Server Windows 2008 R2 client or more recent (more secure)"
            "\n"
            "The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT"
    });

    _.member(MemberInfo{
        .name = names{
            .all = "tls_1_3_ciphersuites",
            .display = "TLS 1.3 cipher suites",
        },
        .value = value<std::string>(""),
        .spec = global_spec(no_acl),
        .desc = tls_1_3_ciphersuites_desc,
    });

    _.member(MemberInfo{
        .name = "tls_key_exchange_groups",
        .value = value<std::string>(""),
        .spec = global_spec(no_acl),
        .desc = tls_key_exchange_groups,
    });

    _.member(MemberInfo{
        .name = "tls_signature_algorithms",
        .value = value<std::string>("RSA+SHA256:RSA+SHA384:RSA+SHA512:RSA-PSS+SHA256:RSA-PSS+SHA384:RSA-PSS+SHA512:ECDSA+SHA256:ECDSA+SHA384:ECDSA+SHA512"),
        .spec = global_spec(no_acl),
        .desc = str_concat(tls_signature_algorithms_desc,
                           tls_signature_algorithms_server_extra_desc),
    });

    _.member(MemberInfo{
        .name = "show_common_cipher_list",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Debug,
        .desc = "Show the common cipher list supported by client and target server in the logs.\n"
        "⚠ Only for debugging purposes.",
    });

    _.member(MemberInfo{
        .name = "enable_nla",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc = "Needed for primary NTLM or Kerberos connections over NLA.",
    });

    _.member(MemberInfo{
        .name = "disable_tsk_switch_shortcuts",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
        .desc = "If enabled, ignore Ctrl+Alt+Del, Ctrl+Shift+Esc and Windows+Tab keyboard sequences.",
    });

    _.member(MemberInfo{
        .name = "rdp_compression",
        .value = from_enum(RdpCompression::rdp6_1),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc = "Specifies the highest RDP compression support available on the client connection session.",
        // RZH: This option can help debugging error during connection in specific cases.
    });

    _.member(MemberInfo{
        .name = "max_color_depth",
        .value = from_enum(ColorDepth::depth24),
        .spec = global_spec(no_acl, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "persistent_disk_bitmap_cache",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc = "Persistent Disk Bitmap Cache on the primary connection side. If the RDP client supports it, this setting increases the size of image caches.",
    });

    _.member(MemberInfo{
        .name = "cache_waiting_list",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc = "Support of Cache Waiting List: Experimental cache strategy (this value is ignored if :REF:[client]:persistent_disk_bitmap_cache is disabled).",
    });

    _.member(MemberInfo{
        .name = "persist_bitmap_cache_on_disk",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc =
            "If enabled, the content of Persistent Bitmap Caches are stored on the disk to reuse later (this value is ignored if :REF:[client]:persistent_disk_bitmap_cache is disabled).",
    });

    _.member(MemberInfo{
        .name = "bitmap_compression",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Compatibility,
        .desc = "Enable Bitmap Compression when supported by the RDP client.\n"
        "Disabling this option increases the network bandwith usage.",
    });

    // TODO remove ?
    _.member(MemberInfo{
        .name = "fast_path",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc = "Enables support of Client Fast-Path Input Event PDUs.",
    });

    _.member(MemberInfo{
        .name = "enable_suppress_output",
        .value = value(true),
        .spec = global_spec(no_acl),
        .tags = Tag::Compatibility,
        .desc =
            "Allows the client to request the target server to stop graphical updates. For example, when the RDP client window is minimized to reduce bandwidth.\n"
            "⚠ If changes occur on the target, they will not be visible in the recordings either."
    });

    _.member(MemberInfo{
        .name = "bogus_ios_glyph_support_level",
        .value = value(true),
        .spec = global_spec(no_acl),
        .tags = Tag::Compatibility,
        .desc = "Same effect as :REF:[client]:transform_glyph_to_bitmap, but only for RDP clients on an iOS platform.",
    });

    // TODO should be merged with disabled_orders
    _.member(MemberInfo{
        .name = "transform_glyph_to_bitmap",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Compatibility,
        .desc = "This option converts glyphs to bitmaps to resolve issues with certain RDP clients."
    });

    _.member(MemberInfo{
        .name = "recv_timeout",
        .value = value<types::range<std::chrono::milliseconds, 100, 10000>>(1000),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "enable_osd_4_eyes",
        .value = value(true),
        .spec = global_spec(no_acl),
        .desc = "Informs users with a message when their session is audited.",
        // Add warning about legal issue specific of the country in use.
    });

    _.member(MemberInfo{
        .name = "enable_remotefx",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf | Tag::Compatibility,
        .desc = "Enable RemoteFX on the client connection.\n"
        "Needs - :REF:[client]:max_color_depth set to 32 (32-bit RGB mask + alpha)\n"
        "      - :REF:[mod_rdp]:enable_remotefx set to on",
    });

    _.member(MemberInfo{
        .name = "disabled_orders",
        .value = value<types::list<types::unsigned_>>("25"),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Debug | Tag::Compatibility,
        .desc = disabled_orders_desc,
    });

    _.member(MemberInfo{
        .name = "workaround_incomplete_images",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Compatibility,
        .desc = "This option fixes a bug in the Remote Desktop client on Windows 11/Windows Server 2025 starting with version 24H2.\n"
        "Occasionally, some screen areas may not refresh due to a bug where only the first image in a BitmapUpdate message with multiple images is displayed correctly.\n"
        "Enabling this option prevents this issue, but will slightly increase the data sent to the client.\n"
        "The option is automatically disabled if the connection is from an Access Manager."
    });
});

_.section("all_target_mod", [&]
{
    _.member(MemberInfo{
        .name = "connection_establishment_timeout",
        .value = value<types::range<std::chrono::milliseconds, 1000, 10000>>(3000),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "The maximum wait time for the proxy to connect to a target.",
    });

    _.member(MemberInfo{
        .name = "tcp_user_timeout",
        .value = value<types::range<std::chrono::milliseconds, 0, 3'600'000>>(),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = "This parameter allows you to specify max timeout before a TCP connection is aborted. If the option value is specified as 0, TCP will use the system default.",
    });
});

_.section("remote_program", [&]
{
    _.member(MemberInfo{
        .name = "allow_resize_hosted_desktop",
        .value = value(true),
        .spec = global_spec(no_acl),
        .desc =
            "Allows resizing of a desktop session opened in a RemoteApp window.\n"
            "This happens when an RDP client opened in RemoteApp accesses a desktop target."
    });
});

_.section(names{.all="mod_rdp", .connpolicy="rdp"}, [&]
{
    _.member(MemberInfo{
        .name = "force_performance_flags",
        .value = value<types::performance_flags>("-mouse_cursor_shadows,-theme,-menu_animations"),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf | Tag::Compatibility,
        .desc =
            "It specifies a list of (comma-separated) RDP server desktop features to enable or disable in the session (with the goal of optimizing bandwidth usage).\n"
            "\n"
            "If a feature is preceded by a \"-\" sign, it is disabled; if it is preceded by a \"+\" sign or no sign, it is enabled. Unconfigured features can be controlled by the RDP client.\n"
            "\n"
            "Available features:\n"
            "  - wallpaper\n"
            "  - full_window_drag\n"
            "  - menu_animations\n"
            "  - theme\n"
            "  - mouse_cursor_shadows\n"
            "  - cursor_blinking\n"
            "  - font_smoothing\n"
            "  - desktop_composition\n"
    });

    _.member(MemberInfo{
        .name = "auto_adjust_performance_flags",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "If enabled, the font smoothing desktop feature is automatically disabled in recorded session.\n"
            "This allows OCR (when session probe is disabled) to better detect window titles.\n"
            "If disabled, it allows font smoothing in recorded sessions. However, OCR will not work when session recording is disabled. In this case, window titles are not detected.",
    });

    _.member(MemberInfo{
        .name = "rdp_compression",
        .value = from_enum(RdpCompression::rdp6_1),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc = "Specifies the highest RDP compression support available on the target server connection.",
        // RZH: This option can help debugging error during connection in specific cases.
    });

    _.member(MemberInfo{
        .name = "disconnect_on_logon_user_change",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "open_session_timeout",
        .value = value<std::chrono::seconds>(),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "The maximum wait time for the proxy to log on to an RDP session.\n"
            "Value 0 is equivalent to 15 seconds."
    });

    _.member(MemberInfo{
        .name = "disabled_orders",
        // disable glyph_index / glyph_cache
        .value = value<types::list<types::unsigned_>>("27"),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Debug | Tag::Compatibility,
        .desc = disabled_orders_desc,
    });

    _.member(MemberInfo{
        .name = "enable_nla",
        .value = value<bool>(true),
        .spec = connpolicy(rdp, loggable),
        .desc = "Enable NLA authentication in secondary targets.",
    });

    _.member(MemberInfo{
        .name = "enable_kerberos",
        .value = value(true, rdp_sogisces_1_3_2030_policy_value(true)),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "When :REF:NOSUFFIX:[mod_rdp]:enable_nla is selected:\n"
            "Enabling this option instructs the Bastion to use Kerberos"
            " as its initial method.\n"
            "Disabling this option instructs the Bastion to only use NTLM.\n"
    });

    _.member(MemberInfo{
        .name = "allow_nla_ntlm_fallback",
        .value = value<bool>(false, rdp_sogisces_1_3_2030_policy_value(false)),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "When both :REF:NOSUFFIX:[mod_rdp]:enable_nla and"
            " :REF:NOSUFFIX:[mod_rdp]:enable_kerberos are selected:\n"
            "Enabling this option instructs the Bastion to use Kerberos first"
            " and, if necessary, NTLM as a backup.\n"
            "Disabling this option instructs the Bastion to only use Kerberos.",
    });

    _.member(MemberInfo{
        .name = "allow_tls_only_fallback",
        .value = value<bool>(false, rdp_sogisces_1_3_2030_policy_value(false)),
        .spec = connpolicy(rdp, loggable),
        .desc = "Allow TLS only."
    });

    _.member(MemberInfo{
        .name = "allow_rdp_legacy_fallback",
        .value = value<bool>(false, rdp_sogisces_1_3_2030_policy_value(false)),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc = "Allow Standard RDP Security (Legacy).",
    });

    _.member(MemberInfo{
        .name = "tls_min_level",
        .value = value<types::u32>(2, rdp_sogisces_1_3_2030_policy_value(2)),
        .spec = connpolicy(rdp, loggable),
        .desc = "Minimal incoming TLS level 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3",
    });

    _.member(MemberInfo{
        .name = "tls_max_level",
        .value = value<types::u32>(0, rdp_sogisces_1_3_2030_policy_value(0)),
        .spec = connpolicy(rdp, loggable),
        .desc = "Maximal incoming TLS level 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3",
    });

    // since openssl 2, the security level is 2
    // "DEFAULT@SECLEVEL=1" (<= 1.1)
    // "DEFAULT@SECLEVEL=2" (>= 2)
    _.member(MemberInfo{
        .name = "cipher_string",
        .value = value<std::string>(
            "ECDHE-ECDSA-AES256-GCM-SHA384"
            ":ECDHE-RSA-AES256-GCM-SHA384"
            ":DHE-RSA-AES256-GCM-SHA384"
            ":ECDHE-ECDSA-AES128-GCM-SHA256"
            ":ECDHE-RSA-AES128-GCM-SHA256"
            ":DHE-RSA-AES128-GCM-SHA256",
            rdp_sogisces_1_3_2030_policy_value(
                "ECDHE-ECDSA-AES256-GCM-SHA384"
                ":ECDHE-RSA-AES256-GCM-SHA384"
                ":DHE-RSA-AES256-GCM-SHA384"
                ":ECDHE-ECDSA-AES128-GCM-SHA256"
                ":ECDHE-RSA-AES128-GCM-SHA256"
                ":DHE-RSA-AES128-GCM-SHA256"
            )),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "TLSv1.2 and below additional ciphers supported.\n"
            "Empty to apply system-wide configuration (SSL security level 2), ALL for support of all ciphers to ensure highest compatibility with target servers.\n"
            "The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT",
    });

    _.member(MemberInfo{
        .name = "tls_enable_legacy_server",
        .value = value<bool>(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
        "Allow TLS legacy insecure renegotiation to unpatched target servers.\n"
        "For Windows Server 2008, requires also to set ALL@SECLEVEL=0 in :REF:[mod_rdp]:cipher_string.\n",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "tls_1_3_ciphersuites",
            .display = "TLS 1.3 cipher suites",
        },
        .value = value<std::string>("",
            rdp_sogisces_1_3_2030_policy_value(
                "TLS_AES_256_GCM_SHA384"
                ":TLS_AES_128_GCM_SHA256")
          ),
        .spec = connpolicy(rdp, loggable),
        .desc = tls_1_3_ciphersuites_desc,
    });

    _.member(MemberInfo{
        .name = "tls_key_exchange_groups",
        .value = value<std::string>(
            "P-256"
            ":P-384"
            ":P-521"
            ":ffdhe3072"
            ":ffdhe4096"
            ":ffdhe6144"
            ":ffdhe8192",
            rdp_sogisces_1_3_2030_policy_value(
                "P-256"
                ":P-384"
                ":P-521"
                ":ffdhe3072"
                ":ffdhe4096"
                ":ffdhe6144"
                ":ffdhe8192"
            )
        ),
        .spec = connpolicy(rdp, loggable),
        .desc = tls_key_exchange_groups,
    });

    _.member(MemberInfo{
        .name = "show_common_cipher_list",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Debug,
        .desc = "Show in the logs the common cipher list supported by client and target server\n"
        "⚠ Only for debugging purposes.",
    });

    _.member(MemberInfo{
        .name = "persistent_disk_bitmap_cache",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc = "Persistent Disk Bitmap Cache on the secondary connection side. If supported by the RDP target server, the size of image caches will be increased.",
    });

    _.member(MemberInfo{
        .name = "cache_waiting_list",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Support of Cache Waiting List (this value is ignored if the :REF:[mod_rdp]:persistent_disk_bitmap_cache is disabled).",
    });

    _.member(MemberInfo{
        .name = "persist_bitmap_cache_on_disk",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Perf,
        .desc = "If enabled, the contents of Persistent Bitmap Caches are stored on disk for reusing them later (this value is ignored if :REF:[mod_rdp]:persistent_disk_bitmap_cache is disabled).",
    });

    _.member(MemberInfo{
        .name = "allowed_channels",
        .value = value<types::list<std::string>>("*"),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc =
            "List of (comma-separated) enabled (static) virtual channel. If character '*' is used as a name then enables everything.\n"
            "Explicit names have higher priority than '*' in :REF::denied_channels."
    });

    _.member(MemberInfo{
        .name = "denied_channels",
        .value = value<types::list<std::string>>(),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc =
            "List of (comma-separated) disabled (static) virtual channel. If character '*' is used as a name then disables everything.\n"
            "Explicit names have higher priority than '*' in :REF::allowed_channels."
    });

    _.member(MemberInfo{
        .name = "allowed_dynamic_channels",
        .value = value<std::string>("*"),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "List of (comma-separated) enabled dynamic virtual channel. If character '*' is used as a name then enables everything.\n"
            "Explicit names have higher priority than '*' in :REF::denied_dynamic_channels."
    });

    _.member(MemberInfo{
        .name = "denied_dynamic_channels",
        .value = value<std::string>(""),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "List of (comma-separated) disabled dynamic virtual channel. If character '*' is used as a name then disables everything.\n"
            "Explicit names have higher priority than '*' in :REF::allowed_dynamic_channels."
    });

    _.member(MemberInfo{
        .name = "disable_coreinput_dynamic_channel",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Compatibility,
        .desc = "If this option is unchecked, keyboard/mouse inputs are transmitted over the dynamic virtual channel.\n"
        "This will cause the session inactivity detection and keyboard input log to malfunction in Windows 11 and Windows Server 2025.",
    });

    _.member(MemberInfo{
        .name = "force_screen_resolution",
        .value = value<ScreenResolution>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Force the screen resolution of a RDP target.\n"
            "This option is ignored when the value is 0x0 or empty.",
    });

    // TODO remove ?
    _.member(MemberInfo{
        .name = "fast_path",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc = "Enables support of Client/Server Fast-Path Input/Update PDUs.\nFast-Path is required for Windows Server 2012 (or more recent).",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "server_redirection_support",
            // TODO rename
            .connpolicy = "server_redirection",
            .display = "Enable Server Redirection Support",
        },
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "The secondary target connection can be redirected to a specific session on another RDP server.",
    });

    _.member(MemberInfo{
        .name = "load_balance_info",
        .value = value<std::string>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Load balancing information.\n"
            "For example 'tsv://MS Terminal Services Plugin.1.Sessions' where 'Sessions' is the name of the targeted RD Collection which works fine."
    });

    _.member(MemberInfo{
        .name = "client_address_sent",
        .value = from_enum(ClientAddressSent::no_address),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Client Address to send to target (in InfoPacket).",
    });

    _.member(MemberInfo{
        .name = "proxy_managed_drives",
        .value = value<types::list<std::string>>(),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc = "Shared directory between proxy and secondary target.\nRequires RDPDR support.",
    });

    _.member(MemberInfo{
        .name = "ignore_auth_channel",
        .value = value(false),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = "auth_channel",
        .value = value<types::fixed_string<7>>("*"),
        .spec = global_spec(no_acl),
        .desc = "Authentication channel used by Auto IT scripts. May be '*' to use default name. Keep empty to disable virtual channel.",
    });

    _.member(MemberInfo{
        .name = "checkout_channel",
        .value = value<types::fixed_string<7>>(),
        .spec = global_spec(no_acl),
        .desc = "Authentication channel used by other scripts. No default name. Keep empty to disable virtual channel.",
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("alternate_shell"),
        .value = value<std::string>(),
        .spec = ini_only(acl_to_proxy(reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("shell_arguments"),
        .value = value<std::string>(),
        .spec = ini_only(acl_to_proxy(reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("shell_working_directory"),
        .value = value<std::string>(),
        .spec = ini_only(acl_to_proxy(reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = "use_client_provided_alternate_shell",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "As far as possible, use client-provided initial program (Alternate Shell).",
    });

    _.member(MemberInfo{
        .name = "use_client_provided_remoteapp",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "As far as possible, use client-provided remote program (RemoteApp).",
    });

    _.member(MemberInfo{
        .name = "use_native_remoteapp_capability",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc = "As far as possible, use native RemoteApp capability.",
    });

    _.member(MemberInfo{
        .name = "application_driver_exe_or_file",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_EXE_OR_FILE)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "application_driver_script_argument",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_SCRIPT_ARGUMENT)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "application_driver_chrome_dt_script",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_DT_SCRIPT)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "application_driver_chrome_uia_script",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_UIA_SCRIPT)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "application_driver_firefox_uia_script",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_FIREFOX_UIA_SCRIPT)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "application_driver_ie_script",
        .value = value<types::fixed_string<256>>(CPP_EXPR(REDEMPTION_CONFIG_APPLICATION_DRIVER_IE_SCRIPT)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "hide_client_name",
        .value = value(false),
        .spec = global_spec(no_acl),
        .desc =
            "Do not transmit the client machine name to the RDP server.\n"
            "If Per-Device licensing mode is configured on the RD host, this Bastion will consume a CAL for all of these connections to the RD host."
    });

    _.member(MemberInfo{
        .name = "use_license_store",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Stores CALs issued by the terminal servers.",
        // TODO: Has it consequences with Device CAL and Users CAL ?
    });

    _.member(MemberInfo{
        .name = "bogus_freerdp_clipboard",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Workaround option to support partial clipboard initialization performed by some versions of FreeRDP.",
    });

    _.member(MemberInfo{
        .name = "bogus_ios_rdpdr_virtual_channel",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Workaround option to disable shared disk for RDP client on iOS platform only.",
    });

    // TODO move to connection policy
    _.member(MemberInfo{
        .name = "bogus_refresh_rect",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .tags = Tag::Compatibility,
        .desc = "Workaround option to fix some drawing issues with Windows Server 2012.\n"
        "Can be disabled when none of the targets are Windows Server 2012.",
    });

    _.member(MemberInfo{
        .name = "enable_rdpdr_data_analysis",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Adds RDPDR channel metadata to session logs. Disabling this option makes shared disks more responsive, but metadata will no longer be collected."
            "if at least one authorization of RDPDR is missing (Printer, ComPort, SmartCard, Drive), then this option is considered enabled."
    });

    _.member(MemberInfo{
        .name = "remoteapp_bypass_legal_notice_delay",
        .value = value<std::chrono::milliseconds>(),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Delay before automatically bypassing Windows's Legal Notice screen in RemoteApp mode.\n"
            "Set to 0 to disable this feature."
    });

    _.member(MemberInfo{
        .name = "remoteapp_bypass_legal_notice_timeout",
        .value = value<std::chrono::milliseconds>(20000),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Time limit to automatically bypass Windows's Legal Notice screen in RemoteApp mode.\n"
            "Set to 0 to disable this feature."
    });

    _.member(MemberInfo{
        .name = "log_only_relevant_clipboard_activities",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Some events such as 'Preferred DropEffect' have no particular meaning. This option allows you to exclude these types of events from the logs.",
    });

    _.member(MemberInfo{
        .name = "split_domain",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Force splitting target domain and username with '@' separator.",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "wabam_uses_translated_remoteapp",
            .display = "Enable translated RemoteApp with AM",
        },
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Actives conversion of RemoteApp target session to desktop session.\n"
            "Otherwise, Alternate Shell will be used.\n"
            "Some Windows Shell features may be unavailable in one or both cases, and applications using them may behave differently."
    });

    _.member(MemberInfo{
        .name = "session_shadowing_support",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Enables Session Shadowing Support.\n"
        "Session probe must be enabled on target connection policy.\n"
        "Target server must support \"Remote Desktop Shadowing\" feature.\n"
        "When enabled, users can share their RDP sessions with auditors who request it.\n",
        // TODO: Move to connection policy ? if so, change reference in admin guide also.
    });

    _.member(MemberInfo{
        .name = "enable_remotefx",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .tags = Tag::Compatibility,
        .desc = "Enables support of the RemoteFX codec on target connection.",
    });

    _.member(MemberInfo{
        .name = "enable_restricted_admin_mode",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Connect to the target server in Restricted Admin mode.\n"
            "This mode must be supported by the target server (available from Windows Server 2012 R2), otherwise, connection will fail.\n"
            "NLA must be enabled."
    });

    _.member(MemberInfo{
        .name = "force_smartcard_authentication",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "NLA will be disabled.\n"
            "Target must be set for interactive login, otherwise the  connection may not be guaranteed.\n"
            "Smartcard device must be available on client desktop.\n"
            "Smartcard redirection (Proxy option RDP_SMARTCARD) must be enabled on service."
    });

    _.member(MemberInfo{
        .name = "enable_ipv6",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc = "Enable target connection on IPv6",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "mode_console",
            .display = "Console mode"
        },
        .value = enum_as_string(RdpModeConsole::allow),
        .spec = connpolicy(rdp, loggable),
        .desc = "Console mode management for targets on Windows Server 2003 (requested with /console or /admin mstsc option).",
    });

    _.member(MemberInfo{
        .name = "auto_reconnection_on_losing_target_link",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Allows the proxy to automatically reconnect to secondary target when a network error occurs.\n"
            "The target server must support reconnection cookie.",
    });

    _.member(MemberInfo{
        .name = "allow_session_reconnection_by_shortcut",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Workaround,
        .desc =
            "If the feature is enabled, the end user can trigger a session disconnection/reconnection with the shortcut Ctrl+F12.\n"
            "This feature should not be used together with the :REF:[session_probe]:end_disconnected_session.\n"
            "The keyboard shortcut is fixed and cannot be changed."
    });

    _.member(MemberInfo{
        .name = "session_reconnection_delay",
        .value = value<types::range<std::chrono::milliseconds, 0, 15000>>(0),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc = "The delay between a session disconnection and the automatic reconnection that follows.",
    });

    _.member(MemberInfo{
        .name = "forward_client_build_number",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Forward the build number advertised by the client to the target server. "
            "If forwarding is disabled a default (static) build number will be sent to the target server."
    });

    _.member(MemberInfo{
        .name = "bogus_monitor_layout_treatment",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "To resolve the session freeze issue with Windows 7/Windows Server 2008 target.",
    });

    _.member(MemberInfo{
        .name = "krb_armoring_account",
        .value = value<std::string>(),
        .spec = acl_connpolicy(rdp, spec::advanced),
        .desc =
            "Account to be used for armoring Kerberos tickets. "
            "Must be in the form 'account_name@domain_name[@device_name]'. "
            "If account resolution succeeds the username and password associated with this account will be used; "
            "otherwise the below fallback username and password will be used instead."
    });

    _.member(MemberInfo{
        .name = "krb_armoring_realm",
        .value = value<std::string>(),
        .spec = acl_connpolicy(rdp, spec::advanced),
        .desc = "Realm to be used for armoring Kerberos tickets.",
    });

    _.member(MemberInfo{
        .name = "krb_armoring_fallback_user",
        .value = value<std::string>(),
        .spec = acl_connpolicy(rdp, spec::advanced),
        .desc = "Fallback username to be used for armoring Kerberos tickets.",
    });

    _.member(MemberInfo{
        .name = "krb_armoring_fallback_password",
        .value = value<std::string>(),
        .spec = acl_connpolicy(rdp, spec::advanced),
        .desc = "Fallback password to be used for armoring Kerberos tickets.",
    });

    _.member(MemberInfo{
        .name = "effective_krb_armoring_user",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
        .desc = "Effective username to be used for armoring Kerberos tickets.",
    });

    _.member(MemberInfo{
        .name = "effective_krb_armoring_password",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, non_loggable),
        .desc = "Effective password to be used for armoring Kerberos tickets.",
    });

    _.member(MemberInfo{
        .name = "remote_programs_disconnect_message_delay",
        .value = value<types::range<std::chrono::milliseconds, 3000, 120000>>(3000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc = "Delay before showing disconnect message after the last RemoteApp window is closed.",
    });

    _.member(MemberInfo{
        .name = "use_session_probe_to_launch_remote_program",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This option only has an effect in RemoteApp sessions (RDS meaning).\n"
            "If enabled, the RDP Proxy relies on the Session Probe to launch the remote programs.\n"
            "Otherwise, remote programs will be launched according to Remote Programs Virtual Channel Extension of Remote Desktop Protocol. This latter is the native method.\n"
            "The difference is that Session Probe does not start a new application when its host session is resumed. Conversely, launching applications according to Remote Programs Virtual Channel Extension of Remote Desktop Protocol is not affected by this behavior. However, launching applications via the native method requires them to be published in Remote Desktop Services, which is unnecessary if launched by the Session Probe."
    });

    _.member(MemberInfo{
        .name = "replace_null_pointer_by_default_pointer",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Workaround,
        .desc = "Replace an empty mouse pointer with normal pointer.",
    });

    _.member(MemberInfo{
        .name = "windows_xp_clipboard_support",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "Enable Windows XP clipboard support.",
    });

    _.member(MemberInfo{
        .name = "block_user_input_until_appdriver_completes",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "Block user input in the session during the execution of Application Driver. Do not activate this option if the application session does not use Application Driver."
    });
});

_.section("protocol", [&]
{
    _.member(MemberInfo{
        .name = "save_session_info_pdu",
        .value = from_enum(RdpSaveSessionInfoPDU::UnsupportedOrUnknown),
        .spec = connpolicy(rdp, loggable, spec::advanced),
    });
});

_.section("session_probe", [&]
{
    _.member(MemberInfo{
        .name = "enable_session_probe",
        .value = value<bool>(false,
            rdp_all_policy_value(true)),
        .spec = connpolicy(rdp, loggable),
    });

    _.member(MemberInfo{
        .name = "exe_or_file",
        .value = value<types::fixed_string<511>>("||CMD"),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "arguments",
        .value = value<types::fixed_string<511>>(CPP_EXPR(REDEMPTION_CONFIG_SESSION_PROBE_ARGUMENTS)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "use_smart_launcher",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter only has an effect in Desktop sessions.\n"
            "It allows you to choose between Smart launcher and Legacy launcher to launch the Session Probe.\n"
            "The Smart launcher and the Legacy launcher do not have the same technical prerequisites. Detailed information can be found in the Administration guide."
    });

    _.member(MemberInfo{
        .name = "enable_launch_mask",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter enables or disables the Session Probe’s launch mask.\n"
            "The Launch mask hides the Session Probe launch steps from the end-users.\n"
            "Disabling the mask makes it easier to diagnose Session Probe launch issues. It is recommended to enable the mask for normal operation."
    });

    _.member(MemberInfo{
        .name = "on_launch_failure",
        .value = from_enum(SessionProbeOnLaunchFailure::disconnect_user),
        .spec = connpolicy(rdp, loggable),
        .desc = "It is recommended to use option 1 (disconnect user).",
    });

    _.member(MemberInfo{
        .name = "launch_timeout",
        .value = value<types::range<std::chrono::milliseconds, 0, 300000>>(40000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter is used if :REF::on_launch_failure is 1 (disconnect user).\n"
            "0 to disable timeout."
    });

    _.member(MemberInfo{
        .name = "launch_fallback_timeout",
        .value = value<types::range<std::chrono::milliseconds, 0, 300000>>(40000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter is used if :REF::on_launch_failure is 0 (ignore failure and continue) or 2 (retry without Session Probe).\n"
            "0 to disable timeout."
    });

    _.member(MemberInfo{
        .name = "start_launch_timeout_timer_only_after_logon",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc = "If enabled, the :REF::launch_timeout countdown timer will be started only after user logged in Windows. Otherwise, the countdown timer will be started immediately after RDP protocol connexion.",
    });

    _.member(MemberInfo{
        .name = "keepalive_timeout",
        .value = value<types::range<std::chrono::milliseconds, 0, 60000>>(5000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The amount of time that RDP Proxy waits for a reply from the Session Probe to the KeepAlive message before adopting the behavior defined by :REF::on_keepalive_timeout.\n"
            "If our local network is subject to congestion, or if the Windows lacks responsiveness, it is possible to increase the value of the timeout to minimize disturbances related to the behavior defined by :REF::on_keepalive_timeout.\n"
            "The KeepAlive message is used to detect Session Probe unavailability. Without Session Probe, session monitoring will be minimal. No metadata will be collected.\n"
            "During the delay between sending a KeepAlive request and receiving the corresponding reply, Session Probe availability is indeterminate."
    });

    _.member(MemberInfo{
        .name = "on_keepalive_timeout",
        .value = from_enum(SessionProbeOnKeepaliveTimeout::freeze_connection_and_wait),
        .spec = connpolicy(rdp, loggable),
        .desc = "This parameter allows us to choose the behavior of the RDP Proxy in case of losing the connection with Session Probe.",
    });

    _.member(MemberInfo{
        .name = "end_disconnected_session",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "The behavior of this parameter is different between the Desktop session and the RemoteApp session (RDS meaning). But in each case, the purpose of enabling this parameter is to not leave disconnected sessions in a state unusable by the RDP proxy.\n"
            "If enabled, Session Probe will automatically end the disconnected Desktop session. Otherwise, the RDP session and the applications it contains will remain active after user disconnection (unless a parameter defined at the RDS-level decides otherwise).\n"
            "The parameter in RemoteApp session (RDS meaning) does not cause the latter to be closed but a simple cleanup. However, this makes the session suitable for reuse.\n"
            "This parameter must be enabled for Web applications because an existing session with a running browser cannot be reused.\n"
            "It is also recommended to enable this parameter for connections in RemoteApp mode (RDS meaning) when :REF:[mod_rdp]:use_session_probe_to_launch_remote_program is enabled. Because an existing Session Probe does not launch a startup program (a new Bastion application) when the RemoteApp session resumes."
    });

    _.member(MemberInfo{
        .name = "enable_autodeployed_appdriver_affinity",
        .value = value(true),
        .spec = connpolicy(rdp, loggable),
        .desc = "If enabled, disconnected auto-deployed Application Driver session will automatically terminate by Session Probe."
    });

    _.member(MemberInfo{
        .name = "enable_log",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter allows you to enable the Windows-side logging of Session Probe.\n"
            "The generated files are located in the Windows user's temporary directory. These files can only be analyzed by the WALLIX team.\n"
            "This log does not help diagnose a launch problem. For this, you should instead use the debugging settings in the Configuration options."
    });

    _.member(MemberInfo{
        .name = "enable_log_rotation",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter enables or disables the Log files rotation for Windows-side logging of Session Probe.\n"
            "The Log files rotation helps reduce disk space consumption caused by logging. But the interesting information may be lost if the corresponding file is not retrieved in time."
    });

    _.member(MemberInfo{
        .name = "log_level",
        .value = from_enum(SessionProbeLogLevel::Debug),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Debug,
        .desc = "Defines logging severity levels.",
    });

    _.member(MemberInfo{
        .name = "disconnected_application_limit",
        .value = value<types::range<std::chrono::milliseconds, 0, 172'800'000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "(Deprecated!)\n"
            "The period above which the disconnected Application session will be automatically closed by the Session Probe.\n"
            "0 to disable timeout."
    });

    _.member(MemberInfo{
        .name = "disconnected_session_limit",
        .value = value<types::range<std::chrono::milliseconds, 0, 172'800'000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The period above which the disconnected Desktop session will be automatically closed by the Session Probe.\n"
            "0 to disable timeout."
    });

    _.member(MemberInfo{
        .name = "idle_session_limit",
        .value = value<types::range<std::chrono::milliseconds, 0, 172'800'000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The period of user inactivity above which the session will be locked by the Session Probe.\n"
            "0 to disable timeout."
    });

    _.member(MemberInfo{
        .name = "smart_launcher_clipboard_initialization_delay",
        .value = value<std::chrono::milliseconds>(2000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The additional period given to the device to make Clipboard redirection available.\n"
            "This parameter is effective only if :REF::use_smart_launcher is enabled.\n"
            "If we see the message \"Clipboard Virtual Channel is unavailable\" in the Bastion’s syslog and we are sure that this virtual channel is allowed on the device (confirmed by a direct connection test for example), we probably need to use this parameter."
    });

    _.member(MemberInfo{
        .name = "smart_launcher_start_delay",
        .value = value<std::chrono::milliseconds>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "For under-performing devices.\n"
            "The extra time given to the device before starting the Session Probe launch sequence.\n"
            "This parameter is effective only if :REF::use_smart_launcher is enabled.\n"
            "This parameter can be useful when (with Launch mask disabled) Windows Explorer is not immediately visible when the RDP session is opened."
    });

    _.member(MemberInfo{
        .name = "smart_launcher_long_delay",
        .value = value<std::chrono::milliseconds>(500),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The delay between two simulated keystrokes during the Session Probe launch sequence execution.\n"
            "This parameter is effective only if :REF::use_smart_launcher is enabled.\n"
            "This parameter may help if the Session Probe launch failure is caused by network slowness or device under-performance.\n"
            "This parameter is usually used together with the :REF::smart_launcher_short_delay."
    });

    _.member(MemberInfo{
        .name = "smart_launcher_short_delay",
        .value = value<std::chrono::milliseconds>(50),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The delay between two steps of the same simulated keystrokes during the Session Probe launch sequence execution.\n"
            "This parameter is effective only if :REF::use_smart_launcher is enabled.\n"
            "This parameter may help if the Session Probe launch failure is caused by network slowness or device under-performance.\n"
            "This parameter is usually used together with the :REF::smart_launcher_long_delay."
    });

    _.member(MemberInfo{
        .name = names{
            .all = "smart_launcher_enable_wabam_affinity",
            .display = "Enable Smart launcher with AM affinity",
        },
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Allow sufficient time for the RDP client (Access Manager) to respond to the Clipboard virtual channel initialization message. Otherwise, the time granted to the RDP client (Access Manager or another) for Clipboard virtual channel initialization will be defined by the :REF::smart_launcher_clipboard_initialization_delay.\n"
            "This parameter is effective only if :REF::use_smart_launcher is enabled and the RDP client is Access Manager."
    });

    _.member(MemberInfo{
        .name = "launcher_abort_delay",
        .value = value<types::range<std::chrono::milliseconds, 0, 300000>>(2000),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "The time interval between the detection of an error (example: a refusal by the target of the redirected drive) and the actual abandonment of the Session Probe launch.\n"
            "The purpose of this parameter is to give the target time to gracefully stop some ongoing processing.\n"
            "It is strongly recommended to keep the default value of this parameter."
    });

    _.member(MemberInfo{
        .name = "enable_crash_dump",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .tags = Tag::Debug,
        .desc =
            "This parameter enables or disables the crash dump generation when the Session Probe encounters a fatal error.\n"
            "The crash dump file is useful for post-modem debugging. It is not designed for normal use.\n"
            "The generated files are located in the Windows user's temporary directory. These files can only be analyzed by the WALLIX team.\n"
            "There is no rotation mechanism to limit the number of dump files produced. Extended activation of this parameter can quickly exhaust disk space."
    });

    _.member(MemberInfo{
        .name = "handle_usage_limit",
        .value = value<types::range<types::u32, 0, 1000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Use only if you see unusually high consumption of system object handles by the Session Probe.\n"
            "The Session Probe will sabotage and then restart it-self if it consumes more handles than what is defined by this parameter.\n"
            "A value of 0 disables this feature.\n"
            "This feature can cause the session to be disconnected if the value of the :REF::on_keepalive_timeout is set to 1 (Disconnect user).\n"
            "If :REF::allow_multiple_handshake is disabled, restarting the Session Probe will cause the session to disconnect."
    });

    _.member(MemberInfo{
        .name = "memory_usage_limit",
        .value = value<types::range<types::u32, 0, 200'000'000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Use only if you see unusually high consumption of memory by the Session Probe.\n"
            "The Session Probe will sabotage and then restart it-self if it consumes more memory than what is defined by this parameter.\n"
            "A value of 0 disables this feature.\n"
            "This feature can cause the session to be disconnected if the value of the :REF::on_keepalive_timeout is set to 1 (Disconnect user).\n"
            "If :REF::allow_multiple_handshake is disabled, restarting the Session Probe will cause the session to disconnect."
    });

    _.member(MemberInfo{
        .name = "cpu_usage_alarm_threshold",
        .value = value<types::range<types::u32, 0, 10000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This debugging feature was created to determine the cause of high CPU consumption by Session Probe in certain environments.\n"
            "As a percentage, the effective alarm threshold is calculated in relation to the reference consumption determined at the start of the program execution. The alarm is deactivated if this value of parameter is less than 200 (200% of reference consumption).\n"
            "When CPU consumption exceeds the allowed limit, debugging information can be collected (if the Windows-side logging is enabled), then Session Probe will sabotage. Additional behavior is defined by :REF::cpu_usage_alarm_action."
    });

    _.member(MemberInfo{
        .name = "cpu_usage_alarm_action",
        .value = from_enum(SessionProbeCPUUsageAlarmAction::Restart),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc = "Additional behavior when CPU consumption exceeds what is allowed. Refer to the :REF::cpu_usage_alarm_threshold.",
    });

    _.member(MemberInfo{
        .name = "end_of_session_check_delay_time",
        .value = value<types::range<std::chrono::milliseconds, 0, 60000>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "For application session only.\n"
            "The delay between the launch of the application and the start of End of session check.\n"
            "Sometimes an application takes a long time to create its window. If the End of session check is start too early, the Session Probe may mistakenly conclude that there is no longer any active process in the session. And without active processes, the application session will be logged off by the Session Probe.\n"
            "'End of session check delay time' allow you to delay the start of End of session check in order to give the application the time to create its window."
    });

    _.member(MemberInfo{
        .name = "ignore_ui_less_processes_during_end_of_session_check",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "For application session only.\n"
            "If enabled, during the End of session check, the processes that do not have a visible window will not be counted as active processes of the session. Without active processes, the application session will be logged off by the Session Probe."
    });

    _.member(MemberInfo{
        .name = "extra_system_processes",
        .value = value<std::string>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter is used to provide the list of (comma-separated) system processes that can be run in the session.\n"
            "Ex.: dllhos.exe,TSTheme.exe\n"
            "Unlike user processes, system processes do not keep the session open. A session with no user process will be automatically closed by Session Probe after starting the End of session check."
    });

    _.member(MemberInfo{
        .name = "childless_window_as_unidentified_input_field",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter concerns the functionality of the Password field detection performed by the Session Probe. This detection is necessary to avoid logging the text entered in the password fields as metadata of session (also known as Session log).\n"
            "Unfortunately, the detection does not work with applications developed in Java, Flash, etc. In order to work around the problem, we will treat the windows of these applications as input fields of unknown type. Therefore, the text entered in these will not be included in the session’s metadata.\n"
            "One of the specifics of these applications is that their main windows do not have any child window from point of view of WIN32 API. Activating this parameter allows this property to be used to detect applications developed in Java or Flash.\n"
            "Refer to the :REF:[session_log]:keyboard_input_masking_level."
    });

    _.member(MemberInfo{
        .name = "windows_of_these_applications_as_unidentified_input_field",
        .value = value<std::string>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Comma-separated process names. (Ex.: chrome.exe,ngf.exe)\n"
            "This parameter concerns the functionality of the Password field detection performed by the Session Probe. This detection is necessary to avoid logging the text entered in the password fields as metadata of session (also known as Session log).\n"
            "Unfortunately, the detection is not infallible. In order to work around the problem, we will treat the windows of these applications as input fields of unknown type. Therefore, the text entered in these will not be included in the session’s metadata.\n"
            "This parameter is used to provide the list of processes whose windows are considered as input fields of unknown type.\n"
            "Refer to the :REF:[session_log]:keyboard_input_masking_level."
    });

    _.member(MemberInfo{
        .name = "update_disabled_features",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter is used when resuming a session hosting a existing Session Probe.\n"
            "If enabled, the Session Probe will activate or deactivate features according to the value of :REF::disabled_features received when resuming its host session. Otherwise, the Session Probe will keep the same set of features that were used during the previous connection.\n"
            "It is recommended to keep the default value of this parameter."
    });

    _.member(MemberInfo{
        .name = "disabled_features",
        .value = from_enum(
            SessionProbeDisabledFeature::chrome_inspection
          | SessionProbeDisabledFeature::firefox_inspection
          | SessionProbeDisabledFeature::group_membership),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter was created to work around some compatibility issues and to limit the CPU load that the Session Probe process causes.\n"
            "If 'Java Acccess Bridge' feature is disabled, data entered in the password field of Java applications may be visible in the metadata.\n"
            "For more information, refer to :REF:[session_log]:keyboard_input_masking_level.\n"
            "For more information, also refer to :REF::childless_window_as_unidentified_input_field and :REF::windows_of_these_applications_as_unidentified_input_field."
            "It is not recommended to deactivate 'MS Active Accessibility' and 'MS UI Automation' at the same time. This configuration will lead to the loss of detection of password input fields. Entries in these fields will be visible as plain text in the session metadata."
    });

    _.member(MemberInfo{
        .name = "enable_bestsafe_interaction",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter has no effect on the device without BestSafe.\n"
            "Is enabled, Session Probe relies on BestSafe to perform the detection of application launches and the detection of outgoing connections.\n"
            "BestSafe has more efficient mechanisms in these tasks than Session Probe.\n"
            "For more information, refer to :REF::outbound_connection_monitoring_rules and :REF::process_monitoring_rules."
    });

    _.member(MemberInfo{
        .name = "on_account_manipulation",
        .value = from_enum(SessionProbeOnAccountManipulation::allow),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter has no effect on the device without BestSafe.\n"
            "BestSafe interaction must be enabled. Refer to :REF::enable_bestsafe_interaction.\n"
            "This parameter allows you to choose the behavior of the RDP Proxy in case of detection of Windows account manipulation.\n"
            "Detectable account manipulations are the creation, deletion of a Windows account, and the addition and deletion of an account from a Windows user group."
    });

    _.member(MemberInfo{
        .name = "alternate_directory_environment_variable",
        .value = value<types::fixed_string<3>>(),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "This parameter is used to indicate the name of an environment variable, to be set on the Windows device, and pointed to a directory (on the device) that can be used to store and start the Session Probe. The environment variable must be available in the Windows user session.\n"
            "The environment variable name is limited to 3 characters or less.\n"
            "By default, the Session Probe will be stored and started from the temporary directory of Windows user.\n"
            "This parameter is useful if a GPO prevents Session Probe from starting from the Windows user's temporary directory."
    });

    _.member(MemberInfo{
        .name = "public_session",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "If enabled, the session, once disconnected, can be resumed by another Bastion user.\n"
            "Except in special cases, this is usually a security problem.\n"
            "By default, a session can only be resumed by the Bastion user who created it."
    });

    _.member(MemberInfo{
        .name = "outbound_connection_monitoring_rules",
        .value = value<std::string>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter is used to provide the list of (comma-separated) rules used to monitor outgoing connections created in the session.\n"
            "(Ex. IPv4 addresses: $deny:192.168.0.0/24:5900,$allow:192.168.0.110:21)\n"
            "(Ex. IPv6 addresses: $deny:2001:0db8:85a3:0000:0000:8a2e:0370:7334:3389,$allow:[20D1:0:3238:DFE1:63::FEFB]:21)\n"
            "(Ex. hostname can be used to resolve to both IPv4 and IPv6 addresses: $allow:host.domain.net:3389)\n"
            "(Ex. for backwards compatibility only: 10.1.0.0/16:22)\n"
            "A global rule that matches all IP addresses/Ports such as \"0.0.0.0/0:*\" is highly unrecommended.\n"
            "Session Probe must be enabled to use this feature.\n"
            "BestSafe can be used to perform detection of outgoing connections created in the session. Refer to :REF::enable_bestsafe_interaction."
    });

    _.member(MemberInfo{
        .name = "process_monitoring_rules",
        .value = value<std::string>(),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "This parameter is used to provide the list of (comma-separated) rules used to monitor the execution of processes in the session.\n"
            "(Ex.: $deny:taskmgr.exe)\n"
            "@ = All child processes of (Bastion) application (Ex.: $deny:@)\n"
            "BestSafe can be used to perform detection of process launched in the session. Refer to :REF::enable_bestsafe_interaction."
    });

    _.member(MemberInfo{
        .name = "customize_executable_name",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "If enabled, a string of random characters will be added to the name of the Session Probe executable.\n"
            "The result could be: SesProbe-5420.exe\n"
            "Some other features automatically enable customization of the Session Probe executable name. Application Driver auto-deployment for example."
    });

    _.member(MemberInfo{
        .name = names{
            .all = "allow_multiple_handshake",
            .display = "Allow multiple handshakes",
        },
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "If enabled, the RDP Proxy accepts to perform the handshake several times during the same RDP session. "
            "Otherwise, any new handshake attempt will interrupt the current session with the display of an alert message."
    });

    _.member(MemberInfo{
        .name = "at_end_of_session_freeze_connection_and_wait",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc =
            "If disabled, the RDP proxy disconnects from the session when the Session Probe reports that the session is about to close (old behavior).\n"
            "The new session end procedure (freeze and wait) prevents another connection from resuming a session that is close to end-of-life."
    });

    _.member(MemberInfo{
        .name = "enable_cleaner",
        .value = value(true),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "clipboard_based_launcher_reset_keyboard_status",
        .value = value(true),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "process_command_line_retrieve_method",
        .value = from_enum(SessionProbeProcessCommandLineRetrieveMethod::both),
        .spec = connpolicy(rdp, loggable, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "periodic_task_run_interval",
        .value = value<types::range<std::chrono::milliseconds, 300, 2000>>(500),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "Time between two polling performed by Session Probe.\n"
            "The parameter is created to adapt the CPU consumption to the performance of the Windows device.\n"
            "The longer this interval, the less detailed the session metadata collection and the lower the CPU consumption."
    });

    _.member(MemberInfo{
        .name = "pause_if_session_is_disconnected",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "If enabled, Session Probe activity will be minimized when the user is disconnected from the session. No metadata will be collected during this time.\n"
            "The purpose of this behavior is to optimize CPU consumption."
    });

    _.member(MemberInfo{
        .name = "monitor_own_resources_consumption",
        .value = value(false),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "If enabled, Session Probe will monitor its own system resource consumption.\n"
            "This feature increases CPU consumption."
    });
});

_.section(names{"server_cert"}, [&]
{
    _.member(MemberInfo{
        .name = "server_cert_store",
        .value = value<bool>(true),
        .spec = connpolicy(rdp | vnc, loggable),
        .desc = "Keep known target server certificates on Bastion",
    });

    _.member(MemberInfo{
        .name = "server_cert_check",
        .value = from_enum(ServerCertCheck::fails_if_no_match_and_succeed_if_no_know),
        .spec = connpolicy(rdp | vnc, loggable),
    });

    _.member(MemberInfo{
        .name = "server_access_allowed_message",
        .value = from_enum(ServerCertNotification::nobody),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = "Warn if check allow connexion to target server.",
    });

    _.member(MemberInfo{
        .name = "server_cert_create_message",
        .value = from_enum(ServerCertNotification::SIEM),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = "Warn that new target server certificate file was created.",
    });

    _.member(MemberInfo{
        .name = "server_cert_success_message",
        .value = from_enum(ServerCertNotification::nobody),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = "Warn that target server certificate file was successfully checked.",
    });

    _.member(MemberInfo{
        .name = "server_cert_failure_message",
        .value = from_enum(ServerCertNotification::SIEM),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = "Warn that target server certificate file checking failed.",
    });

    _.member(MemberInfo{
        .name = "error_message",
        .value = from_enum(ServerCertNotification::SIEM),
        .spec = ini_only(no_acl),
        .desc = "Warn that target server certificate check raised some internal error.",
    });

    _.member(MemberInfo{
        .name = "enable_external_validation",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "external_cert",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "external_response",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
        .desc = "empty string for wait, 'Ok' or error message.",
    });
});

_.section(names{.all="mod_vnc", .connpolicy="vnc"}, [&]
{
    _.member(MemberInfo{
        .name = "clipboard_up",
        .value = value(false),
        .spec = global_spec(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc = "Check this option to enable the clipboard upload (from client to target server).\n"
        "This only supports text data clipboard (not files).",
    });

    _.member(MemberInfo{
        .name = "clipboard_down",
        .value = value(false),
        .spec = global_spec(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc = "Check this option to enable the clipboard download (from target server to client).\n"
        "This only supports text data clipboard (not files).",
    });

    // TODO should be connpolicy and named disabled_encodings (disabled_orders ?)
    _.member(MemberInfo{
        .name = "encodings",
        .value = value<types::list<types::int_>>(),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Sets additional graphics encoding types that will be negotiated with the VNC target server:\n"
            // "  0: Raw\n"
            // "  1: CopyRect\n"
            "  2: RRE\n"
            "  5: HEXTILE\n"
            "  16: ZRLE\n"
            // "  -239 (0xFFFFFF11): Cursor pseudo-encoding"
    });

    // TODO should be with encoding
    _.member(MemberInfo{
        .name = "support_cursor_pseudo_encoding",
        .value = value(true),
        .spec = connpolicy(vnc, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "server_clipboard_encoding_type",
            .acl = "vnc_server_clipboard_encoding_type"
        },
        .value = enum_as_string(ClipboardEncodingType::latin1),
        .spec = global_spec(acl_to_proxy(no_reset_back_to_selector, loggable), spec::advanced),
        .desc = "VNC target server clipboard text data encoding type.",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "bogus_clipboard_infinite_loop",
            .acl = "vnc_bogus_clipboard_infinite_loop"
        },
        .value = from_enum(VncBogusClipboardInfiniteLoop::delayed),
        .spec = global_spec(acl_to_proxy(no_reset_back_to_selector, loggable), spec::advanced),
        .desc =
            "The RDP clipboard is based on a token that indicates who owns data between target server and client. However, some RDP clients, such as FreeRDP, always appropriate this token. This conflicts with VNC, which also appropriates this token, causing clipboard data to be sent in loops.\n"
            "This option indicates the strategy to adopt in such situations."
    });

    _.member(MemberInfo{
        .name = "server_is_macos",
        .value = value(false),
        .spec = connpolicy(vnc, loggable),
    });

    _.member(MemberInfo{
        .name = "server_unix_alt",
        .value = value(false),
        .spec = connpolicy(vnc, loggable),
        .desc = "When disabled, Ctrl + Alt becomes AltGr (Windows behavior).",
    });

    _.member(MemberInfo{
        .name = "enable_ipv6",
        .value = value(true),
        .spec = connpolicy(vnc, loggable),
        .desc = "Enable target connection on IPv6.",
    });


    _.member(MemberInfo{
        .name = "tls_min_level",
        .value = value<types::u32>(2),
        .spec = connpolicy(vnc, loggable),
        .desc = "Minimal incoming TLS level 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3",
    });

    _.member(MemberInfo{
        .name = "tls_max_level",
        .value = value<types::u32>(0),
        .spec = connpolicy(vnc, loggable),
        .desc = "Maximal incoming TLS level 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3",
    });

    // since openssl 2, the security level is 2
    // "DEFAULT@SECLEVEL=1" (<= 1.1)
    // "DEFAULT@SECLEVEL=2" (>= 2)
    _.member(MemberInfo{
        .name = "cipher_string",
        .value = value<std::string>(
            "ECDHE-ECDSA-AES256-GCM-SHA384"
            ":ECDHE-RSA-AES256-GCM-SHA384"
            ":DHE-RSA-AES256-GCM-SHA384"
            ":ECDHE-ECDSA-AES128-GCM-SHA256"
            ":ECDHE-RSA-AES128-GCM-SHA256"
            ":DHE-RSA-AES128-GCM-SHA256"
        ),
        .spec = connpolicy(vnc, loggable),
        .desc =
            "TLSv1.2 and below additional ciphers supported.\n"
            "Empty to apply system-wide configuration (SSL security level 2), ALL for support of all ciphers to ensure highest compatibility with target servers.\n"
            "The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT",
    });

    _.member(MemberInfo{
        .name = "tls_enable_legacy_server",
        .value = value<bool>(false),
        .spec = connpolicy(vnc, loggable, spec::advanced),
        .desc = "Allow TLS legacy insecure renegotiation to unpatched target servers.",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "tls_1_3_ciphersuites",
            .display = "TLS 1.3 cipher suites",
        },
        .value = value<std::string>(""),
        .spec = connpolicy(vnc, loggable),
        .desc = tls_1_3_ciphersuites_desc,
    });

    _.member(MemberInfo{
        .name = "tls_key_exchange_groups",
        .value = value<std::string>(
            "P-256"
            ":P-384"
            ":P-521"
            ":ffdhe3072"
            ":ffdhe4096"
            ":ffdhe6144"
            ":ffdhe8192"
        ),
        .spec = connpolicy(vnc, loggable),
        .desc = tls_key_exchange_groups,
    });

    _.member(MemberInfo{
        .name = "show_common_cipher_list",
        .value = value(false),
        .spec = connpolicy(vnc, loggable, spec::advanced),
        .tags = Tag::Debug,
        .desc = "Show in the logs the common cipher list supported by client and target server.\n"
        "⚠ Only for debugging purposes.",
    });

    _.member(MemberInfo{
        .name = "force_authentication_method",
        .value = value<std::string>(),
        .spec = connpolicy(vnc, loggable, spec::advanced),
        .tags = Tag::Workaround,
        .desc =
            "When specified, force the proxy to use a specific authentication method. "
            "If this method is not supported by the target server, the connection will not be made.\n"
            "  - noauth\n"
            "  - vncauth\n"
            "  - mslogon\n"
            "  - mslogoniiauth\n"
            "  - ultravnc_dsm_old\n"
            "  - ultravnc_dsm_new\n"
            "  - tlsnone\n"
            "  - tlsvnc\n"
            "  - tlsplain\n"
            "  - x509none\n"
            "  - x509vnc\n"
            "  - x509plain",
    });
});

_.section(names{"vnc_over_ssh"}, [&]
{
    _.member(MemberInfo{
        .name = "enable",
        .value = value(false),
        .spec = acl_connpolicy(vnc),
    });

    _.member(MemberInfo{
        .name = "ssh_port",
        .value = value<types::unsigned_>(22),
        .spec = acl_connpolicy(vnc),
        .desc = "Port to be used for SSH tunneling",
    });

    _.member(MemberInfo{
        .name = "tunneling_credential_source",
        .value = enum_as_string(VncTunnelingCredentialSource::scenario_account),
        .spec = acl_connpolicy(vnc),
        .desc =
            "static_login: Static values provided in :REF:[vnc_over_ssh]:ssh_login & :REF:[vnc_over_ssh]:ssh_password fields will be used to establish the SSH tunnel.\n"
            "scenario_account: Scenario account provided in :REF:[vnc_over_ssh]:scenario_account_name field will be used to establish the SSH tunnel. (Recommended)"
    });

    _.member(MemberInfo{
        .name = "ssh_login",
        .value = value<std::string>(),
        .spec = acl_connpolicy(vnc),
        .desc = "Login to be used for SSH tunneling.",
    });

    _.member(MemberInfo{
        .name = "ssh_password",
        .value = value<std::string>(),
        .spec = acl_connpolicy(vnc),
        .desc = "Password to be used for SSH tunneling.",
    });

    _.member(MemberInfo{
        .name = "scenario_account_name",
        .value = value<std::string>(),
        .spec = acl_connpolicy(vnc),
        .desc =
            "With the following syntax: \"account_name@domain_name[@[device_name]]\".\n"
            "\n"
            "Syntax for using global domain scenario account:\n"
            "  \"account_name@global_domain_name\"\n"
            "\n"
            "Syntax for using local domain scenario account (with automatic device name deduction):\n"
            "  \"account_name@local_domain_name@\""
    });

    _.member(MemberInfo{
        .name = "tunneling_type",
        .value = enum_as_string(VncTunnelingType::pxssh),
        .spec = acl_connpolicy(vnc, spec::advanced),
        .desc = "Only for debugging purposes.",
    });
});

_.section("file_verification", [&]
{
    _.member(MemberInfo{
        .name = "socket_path",
        .value = value<std::string>(CPP_EXPR(REDEMPTION_CONFIG_VALIDATOR_PATH)),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "enable_up",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "Enable use of ICAP service for file verification on upload.",
    });

    _.member(MemberInfo{
        .name = "enable_down",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc = "Enable use of ICAP service for file verification on download.",
    });

    _.member(MemberInfo{
        .name = "clipboard_text_up",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Verify text data via clipboard from the client to the target server.\n"
            "File verification on upload must be enabled via the Enable up option."
    });

    _.member(MemberInfo{
        .name = "clipboard_text_down",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Verify text data via clipboard from the target server to the client\n"
            "File verification on download must be enabled via the Enable down option."
    });

    _.member(MemberInfo{
        .name = "block_invalid_file_up",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Block file transfer from the client to the target server on invalid file verification.\n"
            "File verification on upload must be enabled via the Enable up option."
    });

    _.member(MemberInfo{
        .name = "block_invalid_file_down",
        .value = value(false),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Block file transfer from the target server to the client on invalid file verification.\n"
            "File verification on download must be enabled via the Enable down option."
    });

    _.member(MemberInfo{
        .name = "block_invalid_clipboard_text_up",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc =
            "Block text transfer from the client to the target server on invalid text verification.\n"
            "Text verification on upload must be enabled via the Clipboard text up option."
    });

    _.member(MemberInfo{
        .name = "block_invalid_clipboard_text_down",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc =
            "Block text transfer from the target server to the client on invalid text verification.\n"
            "Text verification on download must be enabled via the Clipboard text down option."
    });

    _.member(MemberInfo{
        .name = "log_if_accepted",
        .value = value(true),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc = "Log the files and clipboard texts that are verified and accepted. By default, only those rejected are logged.",
    });

    _.member(MemberInfo{
        .name = "max_file_size_rejected",
        .value = value<types::mebibytes<types::u32>>(256),
        .spec = connpolicy(rdp, loggable, spec::advanced),
        .desc =
            "⚠ This value affects the RAM used by the session.\n\n"
            "If option Block invalid file (up or down) is enabled, automatically reject file with greater filesize."
    });

    _.member(MemberInfo{
        .name = "tmpdir",
        .value = value<types::dirpath>("/tmp/"),
        .spec = ini_only(no_acl),
        .desc = "Temporary path used when files take up too much memory.",
    });
});

_.section("file_storage", [&]
{
    _.member(MemberInfo{
        .name = "store_file",
        .value = enum_as_string(RdpStoreFile::never),
        .spec = connpolicy(rdp, loggable),
        .desc =
            "Enable storage of transferred files (via RDP Clipboard).\n"
            "⚠ Saving files can take up a lot of disk space."
    });
});

// for validator only
for (char const* section_name : {"icap_server_down", "icap_server_up"}) {
    // TODO temporary
    // please, update $REDEMPTION/tools/c++-analyzer/lua-checker/checkers/config.lua for each changement of value
    _.section(section_name, [&]
    {
        _.member(MemberInfo{
            .name = "host",
            .value = value<std::string>(),
            .spec = spec::external(),
            .desc = "IP or FQDN of ICAP server.",
        });

        _.member(MemberInfo{
            .name = "port",
            .value = value<types::unsigned_>(1344),
            .spec = spec::external(),
            .desc = "Port of ICAP server.",
        });

        _.member(MemberInfo{
            .name = "service_name",
            .value = value<std::string>("avscan"),
            .spec = spec::external(),
            .desc = "Service name on ICAP server.",
        });

        _.member(MemberInfo{
            .name = "tls",
            .value = value(false),
            .spec = spec::external(),
            .desc = "Activate TLS on ICAP server connection.",
        });

        _.member(MemberInfo{
            .name = "enable_x_context",
            .value = value(true),
            .spec = spec::external(spec::advanced),
            .desc = "Send X Context (Client-IP, Server-IP, Authenticated-User) to ICAP server.",
        });

        _.member(MemberInfo{
            .name = "filename_percent_encoding",
            .value = value(false),
            .spec = spec::external(spec::advanced),
            .desc = "Filename sent to ICAP as percent encoding.",
        });
    });
}

_.section("mod_replay", [&]
{
    _.member(MemberInfo{
        .name = names{
            .all = "replay_path",
            // normally prefixed
            .acl = "replay_path",
        },
        .value = value<types::dirpath>("/tmp/"),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = "on_end_of_data",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc = "0 - Wait for Escape, 1 - End session",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "replay_on_loop",
            // normally prefixed
            .acl = "replay_on_loop",
        },
        .value = value(false),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
        .desc = "0 - replay once, 1 - loop replay",
    });
});

_.section("ocr", [&]
{
    _.member(MemberInfo{
        .name = "version",
        .value = from_enum(OcrVersion::v2),
        .spec = global_spec(no_acl),
        .desc =
            "Selects the OCR (Optical Character Recognition) version used to detect title bars when Session Probe is not running.\n"
            "Version 1 is a bit faster, but has a higher failure rate in character recognition."
    });

    _.member(MemberInfo{
        .name = "locale",
        .value = enum_as_string(OcrLocale::latin),
        .spec = global_spec(no_acl),
    });

    _.member(MemberInfo{
        .name = "interval",
        .value = value<std::chrono::duration<unsigned, std::centi>>(100),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Time interval between two analyses.\n"
            "A value too low will affect session reactivity."
    });

    _.member(MemberInfo{
        .name = "on_title_bar_only",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Checks shape and color to determine if the text is on a title bar.",
    });

    _.member(MemberInfo{
        .name = "max_unrecog_char_rate",
        .value = value<types::range<types::unsigned_, 0, 100>>(40),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Expressed in percentage,\n"
            "  0   - all of characters need be recognized\n"
            "  100 - accept all results"
    });
});

_.section("capture", [&]
{
    _.member(MemberInfo{
        .name = "record_filebase",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
        .desc = "Basename without extension.",
    });

    _.member(MemberInfo{
        .name = "record_subdirectory",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
        .desc = "Subdirectory of record_path (video section).",
    });

    _.member(MemberInfo{
        .name = "fdx_path",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("hash_path"),
        .value = value<types::dirpath>(CPP_EXPR(app_path(AppPath::Hash))),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("record_tmp_path"),
        .value = value<types::dirpath>(CPP_EXPR(app_path(AppPath::RecordTmp))),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = names::acl_shortname("record_path"),
        .value = value<types::dirpath>(CPP_EXPR(app_path(AppPath::Record))),
        .spec = ini_only(acl_to_proxy(no_reset_back_to_selector, loggable)),
    });

    _.member(MemberInfo{
        .name = "capture_flags",
        .value = from_enum(CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
        .spec = global_spec(no_acl, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "disable_keyboard_log",
        .value = from_enum(KeyboardLogFlags::none,
            // see session_log::keyboard_input_masking_level
            vnc_policy_value(KeyboardLogFlags::session_log | KeyboardLogFlags::wrm)),
        .spec = connpolicy(rdp | vnc, loggable, spec::advanced),
        .desc = {
            "Disable keyboard log:\n"
            "(See also :REF:[session_log]:keyboard_input_masking_level)",
            Description::ConnPolicy{vnc,
                "⚠ Logs may contain passwords.\n\n"
                "Disable keyboard log:"
            },
        }
    });

    _.member(MemberInfo{
        .name = "disable_clipboard_log",
        .value = from_enum(ClipboardLogFlags::none),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Disable clipboard log:",
    });

    _.member(MemberInfo{
        .name = "disable_file_system_log",
        .value = from_enum(FileSystemLogFlags::none),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Disable (redirected) file system log:",
    });

    _.member(MemberInfo{
        .name = "wrm_break_interval",
        .value = value<std::chrono::seconds>(600),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Time between two .wrm recording files.\n"
            "⚠ A value that is too small increases the disk space required for recordings."
    });

    _.member(MemberInfo{
        .name = "wrm_color_depth_selection_strategy",
        .value = from_enum(ColorDepthSelectionStrategy::depth16),
        .spec = global_spec(no_acl, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "wrm_compression_algorithm",
        .value = from_enum(WrmCompressionAlgorithm::gzip),
        .spec = global_spec(no_acl, spec::advanced),
    });

    _.member(MemberInfo{
        .name = "file_permissions",
        .value = value<FilePermissions>(0440),
        .spec = ini_only(no_acl),
        .desc = "Allow to control permissions on recorded files.",
    });
});

_.section("audit", [&]
{
    _.member(MemberInfo{
        .name = "enable_keyboard_log",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Show keyboard input events in the meta file.\n"
            "(See also :REF:[session_log]:keyboard_input_masking_level for RDP and :REF:[capture]:disable_keyboard_log for VNC and RDP)"
    });

    _.member(MemberInfo{
        .name = "video_break_interval",
        .value = value<std::chrono::seconds>(60*60*24*7), // 1 week
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "The maximum time between two videos when no title bar is detected."
    });

    _.member(MemberInfo{
        .name = "video_frame_rate",
        .value = value<types::range<types::unsigned_, 1, 120>>(5),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Maximum number of images per second for video generation.\n"
            "A higher value will produce smoother videos, but the file weight is higher and the generation time longer."
    });

    _.member(MemberInfo{
        .name = "video_notimestamp",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "In the generated video of the session record traces, remove the top left banner with the timestamp.\n"
        "Can slightly speed up the video generation.",
    });

    _.member(MemberInfo{
        .name = "video_codec",
        .value = value<std::string>("mp4"),
        .spec = ini_only(no_acl),
        .desc = "Video codec used for video generation.",
    });

    _.member(MemberInfo{
        .name = "ffmpeg_options",
        .value = value<std::string>("crf=35 preset=superfast"),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "FFmpeg options for video codec. See https://trac.ffmpeg.org/wiki/Encode/H.264\n"
            "⚠ Some browsers and video decoders don't support crf=0"
    });

    _.member(MemberInfo{
        .name = "smart_video_cropping",
        .value = from_enum(SmartVideoCropping::v2),
        .spec = global_spec(no_acl),
    });

    // Detect TS_BITMAP_DATA(Uncompressed bitmap data) + (Compressed)bitmapDataStream
    _.member(MemberInfo{
        .name = "play_video_with_corrupted_bitmap",
        .value = value(false),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Checking this option will allow to play a video with corrupted Bitmap Update.",
    });

    _.member(MemberInfo{
        .name = "file_permissions",
        .value = value<FilePermissions>(0440),
        .spec = ini_only(no_acl),
        .desc = "Allow to control permissions on video files.",
    });

    // real-time conf
    //@{
    _.member(MemberInfo{
        .name = "rt_display",
        .value = value(false),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "allow_rt_without_recording",
        .value = value(false),
        .spec = global_spec(no_acl),
        .desc = "Allow real-time view (4 eyes) without session recording enabled in the authorization.",
    });

    _.member(MemberInfo{
        .name = "rt_png_interval",
        .value = value<std::chrono::milliseconds>(1000),
        .spec = ini_only(no_acl),
        .desc = "Frame interval for 4eyes. A value lower than 6 will have no visible effect.",
    });

    _.member(MemberInfo{
        .name = "rt_png_limit",
        .value = value<types::unsigned_>(5),
        .spec = ini_only(no_acl),
        .desc = "Number of png captures to keep.",
    });
    //@}

    // redis conf
    //@{
    _.member(MemberInfo{
        .name = "use_redis",
        .value = value(true),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "redis_timeout",
        .value = value<std::chrono::milliseconds>(500),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "redis_address",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_port",
        .value = value<types::unsigned_>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_password",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "redis_db",
        .value = value<types::unsigned_>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_use_tls",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_tls_cacert",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_tls_cert",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redis_tls_key",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });
    //@}
});

_.section("crypto", [&]
{
    _.member(MemberInfo{
        .name = "encryption_key",
        .value = value<types::fixed_binary<32>>(default_key),
        .spec = acl_to_proxy(no_reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "sign_key",
        .value = value<types::fixed_binary<32>>(default_key),
        .spec = acl_to_proxy(no_reset_back_to_selector, non_loggable),
    });
});

_.section("websocket", [&]
{
    _.member(MemberInfo{
        .name = "enable_websocket",
        .value = value(false),
        .spec = ini_only(no_acl),
        .desc = "Enable websocket protocol (ws or wss with use_tls=1).",
    });

    _.member(MemberInfo{
        .name = "use_tls",
        .value = value(true),
        .spec = ini_only(no_acl),
        .desc = "Use TLS with websocket (wss).",
    });

    _.member(MemberInfo{
        .name = "listen_address",
        .value = value<std::string>(":3390"),
        .spec = ini_only(no_acl),
        .desc = "${addr}:${port} or ${port} or ${unix_socket_path}",
    });
});

_.section("debug", [&]
{
    _.member(MemberInfo{
        .name = "primary_user",
        .value = value<std::string>(),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Restrict target debugging to a specific primary user.",
    });

    _.member(MemberInfo{
        .name = "fake_target_ip",
        .value = value<std::string>(),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "capture",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_CAPTURE,
    });

    _.member(MemberInfo{
        .name = "auth",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_AUTH,
    });

    _.member(MemberInfo{
        .name = "session",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_SESSION,
    });

    _.member(MemberInfo{
        .name = "front",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_FRONT,
    });

    _.member(MemberInfo{
        .name = "mod_rdp",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_RDP,
    });

    _.member(MemberInfo{
        .name = "mod_vnc",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_VNC,
    });

    _.member(MemberInfo{
        .name = "mod_internal",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_MOD_INTERNAL,
    });

    _.member(MemberInfo{
        .name = "sck_mod",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_SCK,
    });

    _.member(MemberInfo{
        .name = "sck_front",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_SCK,
    });

    _.member(MemberInfo{
        .name = "password",
        .value = value<types::u32>(),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "compression",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_COMPRESSION,
    });

    _.member(MemberInfo{
        .name = "cache",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_CACHE,
    });

    _.member(MemberInfo{
        .name = "ocr",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = CONFIG_DESC_OCR,
    });

    _.member(MemberInfo{
        .name = "ffmpeg",
        .value = value<types::u32>(),
        .spec = global_spec(no_acl, spec::advanced | spec::hex),
        .desc = "Value passed to function av_log_set_level()\nSee https://www.ffmpeg.org/doxygen/2.3/group__lavu__log__constants.html",
    });

    _.member(MemberInfo{
        .name = "config",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Log unknown members or sections.",
    });

    _.member(MemberInfo{
        .name = "mod_rdp_use_failure_simulation_socket_transport",
        .value = from_enum(ModRdpUseFailureSimulationSocketTransport::Off),
        .spec = ini_only(no_acl),
    });

    _.member(MemberInfo{
        .name = "probe_client_addresses",
        .value = value<types::list<std::string>>(),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "List of client probe IP addresses (e.g. ip1, ip2, etc.) to prevent some continuous logs.",
    });
});

_.section("translation", [&]
{
    _.member(MemberInfo{
        .name = names::acl_shortname("language"),
        .value = enum_as_string(Language::en),
        .spec = acl_rw(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "login_language",
        .value = enum_as_string(LoginLanguage::Auto),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "The login page shows this language to all users. Once logged in, users see their preferred language.",
    });
});

_.section("internal_mod", [&]
{
    _.member(MemberInfo{
        .name = "enable_target_field",
        .value = value(true),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = "Allow separate target and login entries by enabling the edit target field on the login page.",
    });

    std::string keyboard_layout_proposals_desc = "<details>"
        "<summary>Possible values in alphabetical order:</summary>"
        "<table><tr><th>Value</th><th>Windows 11 name (informative)</th></tr>";
    for (KeyLayout const& layout : keylayouts_sorted_by_name()) {
        keyboard_layout_proposals_desc += "<tr><td>";
        keyboard_layout_proposals_desc += layout.name;
        keyboard_layout_proposals_desc += "</td><td>";
        keyboard_layout_proposals_desc += layout.display_name;
        keyboard_layout_proposals_desc += "</td></tr>";
    }
    keyboard_layout_proposals_desc += "</table></details>";

    _.member(MemberInfo{
        .name = "keyboard_layout_proposals",
        .value = value<types::list<std::string>>("en-US, fr-FR, de-DE"),
        .spec = global_spec(no_acl, spec::advanced),
        .desc = Description::WithDetails{
            .general = "List of keyboard layouts available by the internal pages button located at bottom left of some internal pages (login, selector, etc).",
            .details = std::move(keyboard_layout_proposals_desc),
        }
    });

    _.member(MemberInfo{
        .name = "enable_close_box",
        .value = value(true),
        .spec = global_spec(no_acl),
        .desc =
            "Display the close screen.\n"
            "Displays secondary connection errors and closes automatically after the specified :REF:[internal_mod]:close_box_timeout value or on user request.",
    });

    _.member(MemberInfo{
        .name = "close_box_timeout",
        .value = value<std::chrono::seconds>(600),
        .spec = global_spec(no_acl, spec::advanced),
        .desc =
            "Specifies the time to spend on the close box of the RDP proxy before closing client window.\n"
            "⚠ Value 0 deactivates the timer and the connection remains open until the client disconnects."
    });
});

_.section("context", [&]
{
    _.member(MemberInfo{
        .name = "psid",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
        .desc = "Proxy session log id",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "opt_bpp",
            .acl = "bpp"
        },
        .value = from_enum(ColorDepth::depth24),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "opt_height",
            .acl = "height"
        },
        .value = value<types::u16>(600),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "opt_width",
            .acl = "width"
        },
        .value = value<types::u16>(800),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "selector",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "selector_current_page",
        .value = value<types::unsigned_>(1),
        .spec = acl_rw(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "selector_device_filter",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "selector_group_filter",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "selector_proto_filter",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "selector_lines_per_page",
        .value = value<types::unsigned_>(0),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "selector_number_of_pages",
        .value = value<types::unsigned_>(1),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_password",
        .value = value<std::string>(),
        .spec = acl_rw(reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "target_host",
        .value = value<std::string>(),
        .spec = acl_rw(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "tunneling_target_host",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_str",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_service",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "target_port",
        .value = value<types::unsigned_>(3389),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "target_protocol",
            .acl = "proto_dest",
        },
        .value = value<std::string>("RDP"),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "password",
        .value = value<std::string>(),
        .spec = acl_rw(no_reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "reporting",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "auth_channel_answer",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable_when_contains_password_string),
    });

    _.member(MemberInfo{
        .name = "auth_channel_target",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "message",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "display_link",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "mod_timeout",
        .value = value<std::chrono::seconds>(0),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "accept_message",
        .value = value(false),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "display_message",
        .value = value(false),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rejected",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "keepalive",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "session_id",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = names{
            .all = "end_date_cnx",
            .acl = "timeclose"
        },
        .value = value<std::chrono::seconds>(0),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "real_target_device",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "authentication_challenge",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "ticket",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "comment",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "duration",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "duration_max",
        .value = value<std::chrono::minutes>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "waitinforeturn",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "formflag",
        .value = value<types::unsigned_>(0),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "module",
        .value = enum_as_string(ModuleName::login),
        .spec = acl_rw(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "try_alternate_target",
        .value = value(false),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "has_more_target",
        .value = value(false),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "proxy_opt",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "pattern_kill",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "pattern_notify",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "opt_message",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "login_message",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "disconnect_reason",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "disconnect_reason_ack",
        .value = value(false),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "ip_target",
        .value = value<std::string>(),
        .spec = no_acl,
    });

    _.member(MemberInfo{
        .name = "recording_started",
        .value = value(false),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rt_ready",
        .value = value(false),
        .spec = acl_rw(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "sharing_ready",
        .value = value(false),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "auth_command",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_notify",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "auth_notify_rail_exec_flags",
        .value = value<types::unsigned_>(0),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "auth_notify_rail_exec_exe_or_file",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_exec_result",
        .value = value<types::u16>(0),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_flags",
        .value = value<types::u16>(0),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_original_exe_or_file",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_exe_or_file",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_working_dir",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_arguments",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_account",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "auth_command_rail_exec_password",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, non_loggable),
    });

    _.member(MemberInfo{
        .name = "is_wabam",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });


    _.member(MemberInfo{
        .name = "pm_response",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "pm_request",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });


    _.member(MemberInfo{
        .name = "native_session_id",
        .value = value<types::u32>(0),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_available",
        .value = value(false),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_userdata",
        .value = value<std::string>(),
        .spec = acl_rw(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_type",
        .value = value<std::string>(),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_invitation_error_code",
        .value = value<types::u32>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_invitation_error_message",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_invitation_id",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_invitation_addr",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rd_shadow_invitation_port",
        .value = value<types::u16>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_userdata",
        .value = value<std::string>(),
        .spec = acl_rw(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "session_sharing_enable_control",
        .value = value(false),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "session_sharing_ttl",
        .value = value<std::chrono::seconds>(600),
        .spec = acl_to_proxy(no_reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "session_sharing_invitation_error_code",
        .value = value<types::u32>(0),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_invitation_error_message",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_invitation_id",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_invitation_addr",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_target_ip",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "session_sharing_target_login",
        .value = value<std::string>(),
        .spec = proxy_to_acl(no_reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "rail_module_host_mod_is_active",
        .value = value(false),
        .spec = no_acl,
    });

    _.member(MemberInfo{
        .name = "smartcard_login",
        .value = value<std::string>(),
        .spec = proxy_to_acl(reset_back_to_selector),
    });

    _.member(MemberInfo{
        .name = "banner_message",
        .value = value<std::string>(),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "banner_type",
        .value = from_enum(BannerType::info),
        .spec = acl_to_proxy(reset_back_to_selector, loggable),
    });

    _.member(MemberInfo{
        .name = "redirection_password_or_cookie",
        .value = value<std::vector<uint8_t>>(),
        .spec = no_acl,
    });
});

_.section("theme", [&]
{
    _.member(MemberInfo{
        .name = "enable_theme",
        .value = value(false),
        .spec = global_spec(no_acl),
        .desc = "Enable custom theme color configuration.",
    });

    _.member(MemberInfo{
        .name = names{
            .all = "logo_path",
            .ini = "logo",
        },
        .value = value<std::string>(CPP_EXPR(REDEMPTION_CONFIG_THEME_LOGO)),
        .spec = global_spec(no_acl, spec::image("/var/wab/images/rdp-oem-logo.png")),
        .desc = "Logo displayed when theme is enabled.",
    });

    auto rgb = [](BGRColor color) {
        return value<types::rgb>(BGRColor(BGRasRGBColor(color)).as_u32());
    };

    _.member(MemberInfo{
        .name = "bgcolor",
        .value = rgb(Theme::Global().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for window, label and button.",
    });

    _.member(MemberInfo{
        .name = "fgcolor",
        .value = rgb(Theme::Global().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for window, label and button.",
    });

    // TODO remove that
    _.member(MemberInfo{
        .name = "separator_color",
        .value = rgb(Theme::Global().separator_color),
        .spec = global_spec(no_acl),
        .desc = "Separator line color used with some widgets.",
    });

    _.member(MemberInfo{
        .name = "focus_color",
        .value = rgb(Theme::Global().focus_color),
        .spec = global_spec(no_acl),
        .desc = "Background color used by buttons when they have focus.",
    });

    _.member(MemberInfo{
        .name = "error_color",
        .value = rgb(Theme::Global().error_color),
        .spec = global_spec(no_acl),
        .desc = "Text color for error messages. For example, an authentication error in the login.",
    });

    _.member(MemberInfo{
        .name = "edit_bgcolor",
        .value = rgb(Theme::Edit().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for editing field.",
    });

    _.member(MemberInfo{
        .name = "edit_fgcolor",
        .value = rgb(Theme::Edit().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for editing field.",
    });

    _.member(MemberInfo{
        .name = "edit_border_color",
        .value = rgb(Theme::Edit().border_color),
        .spec = global_spec(no_acl),
        .desc = "Outline color for editing field.",
    });

    _.member(MemberInfo{
        .name = "edit_focus_border_color",
        .value = rgb(Theme::Edit().focus_border_color),
        .spec = global_spec(no_acl),
        .desc = "Outline color for editing field that has focus.",
    });

    _.member(MemberInfo{
        .name = "edit_cursor_color",
        .value = rgb(Theme::Edit().cursor_color),
        .spec = global_spec(no_acl),
        .desc = "Cursor color for editing field.",
    });

    _.member(MemberInfo{
        .name = "edit_placeholder_color",
        .value = rgb(Theme::Edit().placeholder_color),
        .spec = global_spec(no_acl),
        .desc = "Placeholder text color for editing field with a little resolution.",
    });

    _.member(MemberInfo{
        .name = "password_toggle_color",
        .value = rgb(Theme::Edit().password_toggle_color),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for toggle button of password field.",
    });

    _.member(MemberInfo{
        .name = "tooltip_bgcolor",
        .value = rgb(Theme::Tooltip().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for tooltip.",
    });

    _.member(MemberInfo{
        .name = "tooltip_fgcolor",
        .value = rgb(Theme::Tooltip().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for tooltip.",
    });

    _.member(MemberInfo{
        .name = "tooltip_border_color",
        .value = rgb(Theme::Tooltip().border_color),
        .spec = global_spec(no_acl),
        .desc = "Border color for tooltip.",
    });

    _.member(MemberInfo{
        .name = "selector_line1_bgcolor",
        .value = rgb(Theme::SelectorLine1().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for even rows in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_line1_fgcolor",
        .value = rgb(Theme::SelectorLine1().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for even rows in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_line2_bgcolor",
        .value = rgb(Theme::SelectorLine2().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for odd rows in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_line2_fgcolor",
        .value = rgb(Theme::SelectorLine2().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for odd rows in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_focus_bgcolor",
        .value = rgb(Theme::SelectorFocus().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for the row that has focus in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_focus_fgcolor",
        .value = rgb(Theme::SelectorFocus().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for the row that has focus in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_selected_bgcolor",
        .value = rgb(Theme::SelectorSelected().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for the row that is selected in the selector widget but does not have focus.",
    });

    _.member(MemberInfo{
        .name = "selector_selected_fgcolor",
        .value = rgb(Theme::SelectorSelected().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for the row that is selected in the selector widget but does not have focus.",
    });

    _.member(MemberInfo{
        .name = "selector_label_bgcolor",
        .value = rgb(Theme::SelectorLabel().bgcolor),
        .spec = global_spec(no_acl),
        .desc = "Background color for name of filter fields in the selector widget.",
    });

    _.member(MemberInfo{
        .name = "selector_label_fgcolor",
        .value = rgb(Theme::SelectorLabel().fgcolor),
        .spec = global_spec(no_acl),
        .desc = "Foreground color for name of filter fields in the selector widget.",
    });
});

REDEMPTION_DIAGNOSTIC_POP()

_.build();

}

}
