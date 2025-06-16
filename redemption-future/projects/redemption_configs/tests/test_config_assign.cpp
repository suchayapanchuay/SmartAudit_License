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

#define RED_TEST_MODULE TestStrings
#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "configs/config.hpp"

#include <chrono>


RED_AUTO_TEST_CASE(TestIniAssign)
{
    Inifile ini;

    char const * cpath = "/dsds/dsds";
    char const * cs = "blah";
    char const * cslist = "blah";
    char const * cip = "0.0.0.0";
    std::string s(cs);
    std::string slist(cslist);
    std::string spath(cpath);
    std::string sip(cip);
    unsigned char key[32]{};

    {
        char cs[] = "abc";
        char const c_cs[] = "defef";
        char * pcs = cs;
        char const * pc_cs = c_cs;
        std::string s = "abc";
        std::string const c_s = "defef";
        {
            using type = cfg::context::message;
            ini.set<type>(cs); RED_CHECK_EQUAL(ini.get<type>(), cs);
            ini.set<type>(c_cs); RED_CHECK_EQUAL(ini.get<type>(), c_cs);
            ini.set<type>(pcs); RED_CHECK_EQUAL(ini.get<type>(), pcs);
            ini.set<type>(pc_cs); RED_CHECK_EQUAL(ini.get<type>(), pc_cs);
            ini.set<type>(s); RED_CHECK_EQUAL(ini.get<type>(), s);
            ini.set<type>(c_s); RED_CHECK_EQUAL(ini.get<type>(), c_s);
        }
        {
            using type = cfg::globals::certificate_password;
            ini.set<type>(cs); RED_CHECK_EQUAL(ini.get<type>(), cs);
            ini.set<type>(c_cs); RED_CHECK_EQUAL(ini.get<type>(), c_cs);
            ini.set<type>(pcs); RED_CHECK_EQUAL(ini.get<type>(), pcs);
            ini.set<type>(pc_cs); RED_CHECK_EQUAL(ini.get<type>(), pc_cs);
            ini.set<type>(s); RED_CHECK_EQUAL(ini.get<type>(), s);
            ini.set<type>(c_s); RED_CHECK_EQUAL(ini.get<type>(), c_s);
        }
        {
            using std::begin;
            using std::end;
            using type = cfg::mod_rdp::auth_channel;
            auto & val = ini.get<type>();
            auto first1 = begin(val) + 3;
            auto first2 = begin(val) + 5;
            auto all_null = [&](auto first) { return std::all_of(first, end(val), [](char c) { return !c; }); };
            ini.set<type>(cs); RED_CHECK_EQUAL(val, cs); RED_CHECK(all_null(first1));
            ini.set<type>(c_cs); RED_CHECK_EQUAL(val, c_cs); RED_CHECK(all_null(first2));
            ini.set<type>(pcs); RED_CHECK_EQUAL(val, pcs); RED_CHECK(all_null(first1));
            ini.set<type>(pc_cs); RED_CHECK_EQUAL(val, pc_cs); RED_CHECK(all_null(first2));
            ini.set<type>(s); RED_CHECK_EQUAL(val, s); RED_CHECK(all_null(first1));
            ini.set<type>(c_s); RED_CHECK_EQUAL(val, c_s); RED_CHECK(all_null(first2));
        }
    }

    ini.set<cfg::client::bitmap_compression>(true);
    ini.set<cfg::client::cache_waiting_list>(true);
    ini.set<cfg::client::disable_tsk_switch_shortcuts>(1);
    ini.set<cfg::client::fast_path>(true);
    ini.set<cfg::client::ignore_logon_password>(true);
    ini.set<cfg::internal_mod::keyboard_layout_proposals>(cslist);
    ini.set<cfg::internal_mod::keyboard_layout_proposals>(slist);
    ini.set<cfg::client::max_color_depth>(ColorDepth::depth16);
    ini.set<cfg::client::persist_bitmap_cache_on_disk>(true);
    ini.set<cfg::client::rdp_compression>(RdpCompression::rdp4);
    ini.set<cfg::client::tls_fallback_legacy>(true);
    ini.set<cfg::client::tls_support>(true);

    ini.set_acl<cfg::context::accept_message>(true);
    ini.set<cfg::context::auth_channel_answer>(cs);
    ini.set<cfg::context::auth_channel_answer>(s);
    ini.set_acl<cfg::context::auth_channel_target>(cs);
    ini.set_acl<cfg::context::auth_channel_target>(s);
    ini.set<cfg::context::authentication_challenge>(true);
    ini.set_acl<cfg::context::comment>(cs);
    ini.set_acl<cfg::context::comment>(s);
    ini.set<cfg::crypto::encryption_key>(key);
    // ini.set<cfg::crypto::encryption_key>("12\x00#:\x55", 6u);
    //ini.set<cfg::crypto::encryption_key>("12\x00#:\x55");
    ini.set_acl<cfg::context::display_message>(true);
    ini.set_acl<cfg::context::display_message>(false);
    ini.set_acl<cfg::context::duration>(cs);
    ini.set_acl<cfg::context::duration>(s);
    ini.set<cfg::context::end_date_cnx>(std::chrono::seconds{1});
    ini.set<cfg::context::keepalive>(true);
    ini.set<cfg::context::message>(cs);
    ini.set<cfg::context::message>(s);
    ini.set<cfg::mod_rdp::mode_console>(RdpModeConsole::allow);
    ini.set<cfg::mod_rdp::force_performance_flags>(RdpPerformanceFlags{1, 1});
    ini.set_acl<cfg::context::module>(ModuleName::selector);
    ini.set_acl<cfg::context::opt_bpp>(ColorDepth::depth15);
    ini.set_acl<cfg::context::opt_height>(1);
    ini.set<cfg::context::opt_message>(cs);
    ini.set<cfg::context::opt_message>(s);
    ini.set_acl<cfg::context::opt_width>(1);
    ini.set<cfg::session_probe::outbound_connection_monitoring_rules>(cs);
    ini.set<cfg::session_probe::outbound_connection_monitoring_rules>(s);
    ini.set_acl<cfg::context::password>(cs);
    ini.set_acl<cfg::context::password>(s);
    ini.set<cfg::context::pattern_kill>(cs);
    ini.set<cfg::context::pattern_kill>(cs);
    ini.set<cfg::context::pattern_kill>(s);
    ini.set<cfg::context::pattern_notify>(cs);
    ini.set<cfg::context::pattern_notify>(s);
    ini.set<cfg::context::proxy_opt>(cs);
    ini.set<cfg::context::proxy_opt>(s);
    ini.set_acl<cfg::context::real_target_device>(cs);
    ini.set_acl<cfg::context::real_target_device>(s);
    ini.set_acl<cfg::context::reporting>(cs);
    ini.set_acl<cfg::context::reporting>(s);
    ini.set<cfg::context::selector>(true);
    ini.set_acl<cfg::context::selector_current_page>(1);
    ini.set_acl<cfg::context::selector_device_filter>(cs);
    ini.set_acl<cfg::context::selector_device_filter>(s);
    ini.set_acl<cfg::context::selector_group_filter>(cs);
    ini.set_acl<cfg::context::selector_group_filter>(s);
    ini.set_acl<cfg::context::selector_lines_per_page>(1);
    ini.set<cfg::context::selector_number_of_pages>(1);
    ini.set_acl<cfg::context::selector_proto_filter>(cs);
    ini.set_acl<cfg::context::selector_proto_filter>(s);
    ini.set<cfg::context::session_id>(cs);
    ini.set<cfg::context::session_id>(s);
    ini.set_acl<cfg::context::target_host>(cs);
    ini.set_acl<cfg::context::target_host>(s);
    ini.set_acl<cfg::context::target_password>(cs);
    ini.set_acl<cfg::context::target_password>(s);
    ini.set<cfg::context::target_protocol>(cs);
    ini.set<cfg::context::target_protocol>(s);
    ini.set<cfg::context::target_service>(cs);
    ini.set<cfg::context::target_service>(s);
    ini.set_acl<cfg::context::ticket>(cs);
    ini.set_acl<cfg::context::ticket>(s);
    ini.set_acl<cfg::context::waitinforeturn>(cs);
    ini.set_acl<cfg::context::waitinforeturn>(s);

    // ini.set<cfg::crypto::encryption_key>("12\x00#:\x55", 6u);
    //ini.set<cfg::crypto::encryption_key>("12\x00#:\x55");
    ini.set<cfg::crypto::encryption_key>(key);
    // is_same<key0:type, key1:type>
    [](cfg::crypto::encryption_key::type *){}(static_cast<cfg::crypto::sign_key::type *>(nullptr));

    ini.set_acl<cfg::globals::auth_user>(cs);
    ini.set_acl<cfg::globals::auth_user>(s);
    ini.set<cfg::globals::authfile>(cs);
    ini.set<cfg::globals::authfile>(s);
    ini.set<cfg::globals::certificate_password>(cs);
    ini.set<cfg::globals::certificate_password>(s);
    ini.set<cfg::internal_mod::close_box_timeout>(std::chrono::seconds{1});
    ini.set<cfg::internal_mod::enable_close_box>(true);
    ini.set<cfg::globals::device_id>(cs);
    ini.set<cfg::globals::device_id>(s);
    ini.set<cfg::globals::enable_wab_integration>(true);
    ini.set<cfg::globals::enable_bitmap_update>(true);
    ini.set<cfg::globals::enable_transparent_mode>(true);
    ini.set<cfg::globals::enable_end_time_warning_osd>(true);
    ini.set<cfg::globals::enable_osd_display_remote_target>(true);
    ini.set<cfg::client::encryption_level>(RdpSecurityEncryptionLevel::high);
    ini.set_acl<cfg::globals::host>(cs);
    ini.set_acl<cfg::globals::host>(s);
    ini.set<cfg::globals::keepalive_grace_delay>(std::chrono::minutes{1});
    ini.set<cfg::globals::keepalive_grace_delay>(std::chrono::seconds{1});
    ini.set<cfg::globals::listen_address>(cip);
    ini.set<cfg::globals::listen_address>(sip);
    ini.set<cfg::globals::is_rec>(true);
    ini.set<cfg::audit::video_notimestamp>(true);
    ini.set<cfg::globals::port>(1);
    ini.set<cfg::globals::base_inactivity_timeout>(std::chrono::seconds{1});
    ini.set_acl<cfg::globals::target>(cs);
    ini.set_acl<cfg::globals::target>(s);
    ini.set<cfg::globals::target_application>(cs);
    ini.set<cfg::globals::target_application>(s);
    ini.set<cfg::globals::target_application_account>(cs);
    ini.set<cfg::globals::target_application_account>(s);
    ini.set<cfg::globals::target_application_password>(cs);
    ini.set<cfg::globals::target_application_password>(s);
    ini.set<cfg::globals::target_device>(cs);
    ini.set<cfg::globals::target_device>(s);
    ini.set_acl<cfg::globals::target_user>(cs);
    ini.set_acl<cfg::globals::target_user>(s);
    ini.set<cfg::globals::trace_type>(TraceType::localfile);

    ini.set<cfg::mod_rdp::allowed_channels>(cslist);
    ini.set<cfg::mod_rdp::allowed_channels>(slist);
    ini.set<cfg::mod_rdp::alternate_shell>(cs);
    ini.set<cfg::mod_rdp::alternate_shell>(s);
    ini.set<cfg::mod_rdp::auth_channel>(cs);
    ini.set<cfg::mod_rdp::auth_channel>(s);
    ini.set<cfg::mod_rdp::cache_waiting_list>(true);
    ini.set<cfg::mod_rdp::denied_channels>(cslist);
    ini.set<cfg::mod_rdp::denied_channels>(slist);
    ini.set<cfg::mod_rdp::disconnect_on_logon_user_change>(true);
    ini.set<cfg::mod_rdp::enable_kerberos>(true);
    ini.set<cfg::mod_rdp::enable_nla>(true);
    ini.set<cfg::session_probe::enable_session_probe>(true);
    ini.set<cfg::session_probe::enable_launch_mask>(true);
    ini.set<cfg::mod_rdp::fast_path>(true);
    ini.set<cfg::mod_rdp::ignore_auth_channel>(true);
    ini.set<cfg::mod_rdp::open_session_timeout>(std::chrono::seconds{1});
    ini.set<cfg::mod_rdp::persist_bitmap_cache_on_disk>(true);
    ini.set<cfg::mod_rdp::persistent_disk_bitmap_cache>(true);
    ini.set<cfg::mod_rdp::proxy_managed_drives>(cslist);
    ini.set<cfg::mod_rdp::proxy_managed_drives>(slist);
    ini.set<cfg::mod_rdp::rdp_compression>(RdpCompression::rdp4);
    ini.set<cfg::server_cert::server_cert_check>(ServerCertCheck::always_succeed);
    ini.set<cfg::server_cert::server_cert_store>(true);
    ini.set<cfg::mod_rdp::server_redirection_support>(true);
    ini.set<cfg::session_probe::exe_or_file>(s);
    ini.set<cfg::session_probe::arguments>(s);
    ini.set<cfg::session_probe::customize_executable_name>(true);
    ini.set<cfg::session_probe::end_disconnected_session>(true);
    ini.set<cfg::session_probe::keepalive_timeout>(std::chrono::seconds{1});
    ini.set<cfg::session_probe::launch_timeout>(std::chrono::seconds{1});
    ini.set<cfg::session_probe::on_launch_failure>(SessionProbeOnLaunchFailure::ignore_and_continue);
    ini.set<cfg::mod_rdp::shell_working_directory>(cs);
    ini.set<cfg::mod_rdp::shell_working_directory>(s);

    ini.set<cfg::mod_replay::on_end_of_data>(true);

    ini.set<cfg::mod_vnc::bogus_clipboard_infinite_loop>(VncBogusClipboardInfiniteLoop::duplicated);
    ini.set<cfg::mod_vnc::clipboard_down>(true);
    ini.set<cfg::mod_vnc::clipboard_up>(true);
    ini.set<cfg::mod_vnc::encodings>(cslist);
    ini.set<cfg::mod_vnc::encodings>(slist);
    ini.set<cfg::mod_vnc::server_clipboard_encoding_type>(ClipboardEncodingType::latin1);

    ini.set<cfg::session_log::keyboard_input_masking_level>(KeyboardInputMaskingLevel::unmasked);

    ini.set_acl<cfg::translation::language>(Language::en);
    ini.set<cfg::translation::login_language>(LoginLanguage::Auto);

    ini.set<cfg::capture::wrm_break_interval>(std::chrono::seconds{1});
    ini.set<cfg::capture::capture_flags>(CaptureFlags::wrm | CaptureFlags::png);
    ini.set<cfg::capture::hash_path>(cpath);
    ini.set<cfg::capture::hash_path>(spath);
    ini.set<cfg::audit::rt_png_interval>(std::chrono::seconds{1});
    ini.set<cfg::audit::rt_png_limit>(1);
    ini.set<cfg::capture::record_path>(cpath);
    ini.set<cfg::capture::record_path>(spath);
    ini.set<cfg::capture::record_tmp_path>(cpath);
    ini.set<cfg::capture::record_tmp_path>(spath);
    ini.set<cfg::mod_replay::replay_path>(cpath);
    ini.set<cfg::mod_replay::replay_path>(spath);
    ini.set<cfg::capture::wrm_color_depth_selection_strategy>(ColorDepthSelectionStrategy::depth16);
    ini.set<cfg::capture::wrm_compression_algorithm>(WrmCompressionAlgorithm::gzip);

    ini.set<cfg::audit::rt_display>(1);
}
