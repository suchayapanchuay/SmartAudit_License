/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou
*/


#pragma once

#include "core/RDP/caches/bmpcache.hpp"
#include "core/RDP/capabilities/order.hpp"
#include "core/RDP/windows_execute_shell_params.hpp"
#include "core/channel_names.hpp"
#include "core/server_cert_params.hpp"
#include "keyboard/kbdtypes.hpp"
#include "mod/rdp/channels/validator_params.hpp"
#include "mod/rdp/rdp_verbose.hpp"
#include "utils/sugar/zstring_view.hpp"
#include "utils/log.hpp"
#include "utils/ref.hpp"

#ifndef __EMSCRIPTEN__
# include "mod/rdp/params/rdp_session_probe_params.hpp"
#endif

# include "mod/rdp/params/rdp_application_params.hpp"

#include <chrono>
#include <string>
#include <cstdint>


class ClientExecute;
class Transport;
class Theme;
class Font;

struct ModRDPParams
{
#ifndef __EMSCRIPTEN__
    ModRdpSessionProbeParams session_probe_params;
#endif

    const char * target_user;
    const char * target_password;
    const char * target_host;
    const char * client_address;
    const char * krb_armoring_user = nullptr;
    const char * krb_armoring_password = nullptr;
    const char * krb_armoring_keytab_path = nullptr;

    bool allow_nla_ntlm = true;
    bool allow_tls_only = true;
    bool allow_rdp_legacy = true;
    bool enable_nla = true;
    bool enable_krb = false;
    bool enable_fastpath = true;           // If true, fast-path must be supported.
    bool enable_remotefx = false;

    bool enable_restricted_admin_mode = false;

    ValidatorParams validator_params;

    struct ClipboardParams
    {
        bool log_only_relevant_activities = true;
    };

    ClipboardParams clipboard_params;

    struct FileSystemParams
    {
        bool bogus_ios_rdpdr_virtual_channel = true;
        bool enable_rdpdr_data_analysis = true;
        bool smartcard_passthrough = false;
    };

    FileSystemParams file_system_params;

    Transport  * persistent_key_list_transport = nullptr;

    kbdtypes::KeyLocks key_locks;

    bool         ignore_auth_channel = false;
    CHANNELS::ChannelNameId auth_channel;

    CHANNELS::ChannelNameId checkout_channel;

    ApplicationParams application_params;

    RdpCompression rdp_compression = RdpCompression::none;

    std::chrono::seconds open_session_timeout {};
    bool                 disconnect_on_logon_user_change = false;

    ServerCertParams server_cert_params;

    bool enable_server_cert_external_validation = false;

    bool hide_client_name = false;

    const char * device_id = "";

    PrimaryDrawingOrdersSupport disabled_orders {};

    bool enable_persistent_disk_bitmap_cache = false;
    bool enable_cache_waiting_list = false;
    bool persist_bitmap_cache_on_disk = false;

    uint32_t password_printing_mode = 0;

    bool bogus_freerdp_clipboard = false;

    bool bogus_refresh_rect = true;

    struct DriveParams
    {
        const char * proxy_managed_drives = "";
        const char * proxy_managed_prefix = "";
    };

    DriveParams drive_params;

    Translator translator;

    bool allow_using_multiple_monitors = false;
    bool bogus_monitor_layout_treatment = false;
    bool allow_scale_factor = false;

    bool adjust_performance_flags_for_recording = false;

    struct RemoteAppParams
    {
        bool enable_remote_program   = false;
        bool remote_program_enhanced = false;

        bool convert_remoteapp_to_desktop = false;

        bool use_client_provided_remoteapp = false;

        bool should_ignore_first_client_execute = false;

        WindowsExecuteShellParams windows_execute_shell_params;

        ClientExecute * rail_client_execute = nullptr;
        std::chrono::milliseconds rail_disconnect_message_delay {};

        std::chrono::milliseconds bypass_legal_notice_delay {};
        std::chrono::milliseconds bypass_legal_notice_timeout {};
    };

    RemoteAppParams remote_app_params;

    Ref<Font const> font;
    Theme const & theme;

    bool clean_up_32_bpp_cursor = false;

    bool replace_null_pointer_by_default_pointer = false;

    bool large_pointer_support = true;

    bool perform_automatic_reconnection = false;
    std::array<uint8_t, 28>& server_auto_reconnect_packet_ref;

    const char * load_balance_info = "";

    bool support_connection_redirection_during_recording = true;

    bool split_domain = false;

    bool use_license_store = true;

    struct DynamicChannelsParams
    {
        std::string allowed_channels = "*";
        std::string denied_channels  = "";
    };

    DynamicChannelsParams dynamic_channels_params;

    bool allow_session_reconnection_by_shortcut = false;

    struct ServerInfo
    {
        uint16_t input_flags = 0;
        uint32_t keyboard_layout = 0;
    };

    ServerInfo* server_info_ref = nullptr;

    std::vector<uint8_t> redirection_password_or_cookie;

    RdpSaveSessionInfoPDU save_session_info_pdu = RdpSaveSessionInfoPDU::UnsupportedOrUnknown;

    RDPVerbose verbose;
    BmpCache::Verbose cache_verbose = BmpCache::Verbose::none;

    /// Note: may be required for correct smartcard support; see issue #27767.
    bool forward_client_build_number = true;

    bool windows_xp_clipboard_support = false;

    bool block_user_input_until_appdriver_completes = false;

    ModRDPParams( const char * target_user
                , const char * target_password
                , const char * target_host
                , const char * client_address
                , kbdtypes::KeyLocks key_locks
                , Font const & font
                , Theme const & theme
                , std::array<uint8_t, 28>& server_auto_reconnect_packet_ref
                , std::vector<uint8_t>&& redirection_password_or_cookie
                , Translator translator
                , RDPVerbose verbose
                )
        : target_user(target_user)
        , target_password(target_password)
        , target_host(target_host)
        , client_address(client_address)
        , key_locks(key_locks)
        , translator(translator)
        , font(font)
        , theme(theme)
        , server_auto_reconnect_packet_ref(server_auto_reconnect_packet_ref)
        , redirection_password_or_cookie(std::move(redirection_password_or_cookie))
        , verbose(verbose)
    { }

    void log() const
    {
        auto yes_or_no = [](bool x) -> char const * { return x ? "yes" : "no"; };
        auto hidden_or_null = [](bool x) -> char const * { return x ? "<hidden>" : "<null>"; };
        auto av_hidden_or_null = [](chars_view s) -> char const * { return s.empty() ? "<hidden>" : "<null>"; };
        auto s_or_null = [](char const * s) -> char const * { return s ? s : "<null>"; };
        auto s_or_none = [](char const * s) -> char const * { return s ? s : "<none>"; };
        auto from_sec = [](std::chrono::seconds sec) { return static_cast<unsigned>(sec.count()); };
        auto from_millisec = [](std::chrono::milliseconds millisec) { return static_cast<unsigned>(millisec.count()); };

#define RDP_PARAMS_LOG(format, get, member) \
    LOG(LOG_INFO, "ModRDPParams " #member "=" format, get (this->member))
#define RDP_PARAMS_LOG_AV(av) int(av.size()), av.data()
#define RDP_PARAMS_LOG_GET
        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    target_user);
        RDP_PARAMS_LOG("\"%s\"", hidden_or_null,        target_password);
        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    target_host);
        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    client_address);
        RDP_PARAMS_LOG("\"%s\"", s_or_null,             krb_armoring_user);
        RDP_PARAMS_LOG("\"%s\"", hidden_or_null,        krb_armoring_password);
        RDP_PARAMS_LOG("\"%s\"", s_or_null,             krb_armoring_keytab_path);

        RDP_PARAMS_LOG("\"%.*s\"", RDP_PARAMS_LOG_AV,   application_params.primary_user_id);
        RDP_PARAMS_LOG("\"%.*s\"", RDP_PARAMS_LOG_AV,   application_params.target_application);

        RDP_PARAMS_LOG("%" PRIx32, RDP_PARAMS_LOG_GET,  disabled_orders.as_uint());
        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_nla_ntlm);
        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_tls_only);
        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_rdp_legacy);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_nla);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_krb);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_fastpath);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_remotefx);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_restricted_admin_mode);

#ifndef __EMSCRIPTEN__
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.enable_session_probe);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.enable_launch_mask);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.used_clipboard_based_launcher);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.start_launch_timeout_timer_only_after_logon);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.effective_launch_timeout);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      session_probe_params.vc_params.on_launch_failure);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.keepalive_timeout);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      session_probe_params.vc_params.on_keepalive_timeout);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.end_disconnected_session);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.disconnected_application_limit);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.disconnected_session_limit);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.idle_session_limit);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.customize_executable_name);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.enable_log_rotation);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      session_probe_params.vc_params.log_level);

        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.clipboard_based_launcher.clipboard_initialization_delay_ms);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.clipboard_based_launcher.start_delay_ms);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.clipboard_based_launcher.long_delay_ms);
        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.clipboard_based_launcher.short_delay_ms);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.allow_multiple_handshake);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.enable_crash_dump);

        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, session_probe_params.vc_params.handle_usage_limit);
        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, session_probe_params.vc_params.memory_usage_limit);

        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, session_probe_params.vc_params.cpu_usage_alarm_threshold);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      session_probe_params.vc_params.cpu_usage_alarm_action);

        RDP_PARAMS_LOG("0x%08X", static_cast<unsigned>, session_probe_params.vc_params.disabled_features);

        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.end_of_session_check_delay_time);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.ignore_ui_less_processes_during_end_of_session_check);
        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.update_disabled_features);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.childless_window_as_unidentified_input_field);

        RDP_PARAMS_LOG("%u",     from_millisec,         session_probe_params.vc_params.launcher_abort_delay);

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    session_probe_params.vc_params.extra_system_processes.to_string());

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    session_probe_params.vc_params.outbound_connection_monitor_rules.to_string());

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    session_probe_params.vc_params.process_monitor_rules.to_string());

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    session_probe_params.vc_params.windows_of_these_applications_as_unidentified_input_field.to_string());

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.is_public_session);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.used_to_launch_remote_program);

        RDP_PARAMS_LOG("%s",     yes_or_no,             session_probe_params.vc_params.session_shadowing_support);

        RDP_PARAMS_LOG("%d",     static_cast<int>,      session_probe_params.vc_params.on_account_manipulation);

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    session_probe_params.alternate_directory_environment_variable);

        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    session_probe_params.vc_params.target_ip);
#endif

        RDP_PARAMS_LOG("<%p>",   static_cast<void*>,    persistent_key_list_transport);

        RDP_PARAMS_LOG("%02x",   unsigned,              key_locks);

        RDP_PARAMS_LOG("%s",     yes_or_no,             ignore_auth_channel);
        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    auth_channel);

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    checkout_channel);

        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    application_params.alternate_shell);
        RDP_PARAMS_LOG("\"%s\"", RDP_PARAMS_LOG_GET,    application_params.shell_arguments);
        RDP_PARAMS_LOG("\"%.*s\"", RDP_PARAMS_LOG_AV,   application_params.shell_working_dir);
        RDP_PARAMS_LOG("%s",     yes_or_no,             application_params.use_client_provided_alternate_shell);
        RDP_PARAMS_LOG("\"%.*s\"", RDP_PARAMS_LOG_AV,   application_params.target_application_account);
        RDP_PARAMS_LOG("\"%s\"", av_hidden_or_null,     application_params.target_application_password);

        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, rdp_compression);

        RDP_PARAMS_LOG("%s",     yes_or_no,             disconnect_on_logon_user_change);
        RDP_PARAMS_LOG("%u",     from_sec,              open_session_timeout);

        RDP_PARAMS_LOG("%s",     yes_or_no,             server_cert_params.store);
        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, server_cert_params.check);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      server_cert_params.notifications.access_allowed_message);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      server_cert_params.notifications.create_message);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      server_cert_params.notifications.success_message);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      server_cert_params.notifications.failure_message);
        RDP_PARAMS_LOG("%d",     static_cast<int>,      server_cert_params.notifications.error_message);

        RDP_PARAMS_LOG("%s",     yes_or_no,             hide_client_name);

        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_persistent_disk_bitmap_cache);
        RDP_PARAMS_LOG("%s",     yes_or_no,             enable_cache_waiting_list);
        RDP_PARAMS_LOG("%s",     yes_or_no,             persist_bitmap_cache_on_disk);

        RDP_PARAMS_LOG("%u",     RDP_PARAMS_LOG_GET,    password_printing_mode);

        RDP_PARAMS_LOG("%s",     yes_or_no,             bogus_freerdp_clipboard);
        RDP_PARAMS_LOG("%s",     yes_or_no,             bogus_refresh_rect);

        RDP_PARAMS_LOG("%s",     s_or_none,             drive_params.proxy_managed_drives);
        RDP_PARAMS_LOG("%s",     s_or_none,             drive_params.proxy_managed_prefix);

        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_using_multiple_monitors);
        RDP_PARAMS_LOG("%s",     yes_or_no,             bogus_monitor_layout_treatment);
        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_scale_factor);

        RDP_PARAMS_LOG("%s",     yes_or_no,             adjust_performance_flags_for_recording);

        RDP_PARAMS_LOG("<%p>",   static_cast<void*>,    remote_app_params.rail_client_execute);

        RDP_PARAMS_LOG("0x%04X", RDP_PARAMS_LOG_GET,    remote_app_params.windows_execute_shell_params.flags);

        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    remote_app_params.windows_execute_shell_params.exe_or_file);
        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    remote_app_params.windows_execute_shell_params.working_dir);
        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    remote_app_params.windows_execute_shell_params.arguments);

        RDP_PARAMS_LOG("%s",     yes_or_no,             remote_app_params.use_client_provided_remoteapp);

        RDP_PARAMS_LOG("%s",     yes_or_no,             remote_app_params.should_ignore_first_client_execute);

        RDP_PARAMS_LOG("%s",     yes_or_no,             remote_app_params.enable_remote_program);
        RDP_PARAMS_LOG("%s",     yes_or_no,             remote_app_params.remote_program_enhanced);

        RDP_PARAMS_LOG("%s",     yes_or_no,             remote_app_params.convert_remoteapp_to_desktop);

        RDP_PARAMS_LOG("%s",     yes_or_no,             clean_up_32_bpp_cursor);

        RDP_PARAMS_LOG("%s",     yes_or_no,             large_pointer_support);

        RDP_PARAMS_LOG("%s",     s_or_none,             load_balance_info);

        RDP_PARAMS_LOG("%u",     from_millisec,         remote_app_params.rail_disconnect_message_delay);

        RDP_PARAMS_LOG("%s",     yes_or_no,             file_system_params.bogus_ios_rdpdr_virtual_channel);

        RDP_PARAMS_LOG("%s",     yes_or_no,             file_system_params.enable_rdpdr_data_analysis);

        RDP_PARAMS_LOG("%s",     yes_or_no,             file_system_params.smartcard_passthrough);

        RDP_PARAMS_LOG("%u",     from_millisec,         remote_app_params.bypass_legal_notice_delay);
        RDP_PARAMS_LOG("%u",     from_millisec,         remote_app_params.bypass_legal_notice_timeout);

        RDP_PARAMS_LOG("%s",     yes_or_no,             support_connection_redirection_during_recording);

        RDP_PARAMS_LOG("%s",     yes_or_no,             clipboard_params.log_only_relevant_activities);

        RDP_PARAMS_LOG("%s",     yes_or_no,             use_license_store);

        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    dynamic_channels_params.allowed_channels);
        RDP_PARAMS_LOG("%s",     RDP_PARAMS_LOG_GET,    dynamic_channels_params.denied_channels);

        RDP_PARAMS_LOG("%u",     static_cast<unsigned>, save_session_info_pdu);

        RDP_PARAMS_LOG("%s",     yes_or_no,             allow_session_reconnection_by_shortcut);

        RDP_PARAMS_LOG("%s",     yes_or_no,             windows_xp_clipboard_support);

        RDP_PARAMS_LOG("%s",     yes_or_no,             block_user_input_until_appdriver_completes);

        RDP_PARAMS_LOG("0x%08X", static_cast<unsigned>, verbose);
        RDP_PARAMS_LOG("0x%08X", static_cast<unsigned>, cache_verbose);

#undef RDP_PARAMS_LOG
#undef RDP_PARAMS_LOG_AV
#undef RDP_PARAMS_LOG_GET
    }   // void log() const
};  // struct ModRDPParams
