/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Product name: redemption, a FLOSS RDP proxy
  Copyright (C) Wallix 2012-2013
  Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan

  Unit test to config.cpp file
  Using lib boost functions, some tests need to be added
*/

#define RED_TEST_MODULE TestConfig
#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"

#include "configs/config.hpp"
#include "configs/autogen/str_authid.hpp"
#include "utils/theme.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace
{
    Inifile::FieldReference get_acl_field(Inifile& ini, configs::authid_t id)
    {
        return ini.get_acl_field_by_name(configs::authstr[unsigned(id)]);
    }
}


RED_AUTO_TEST_CASE_WF(TestLoadDefaultIni, wf)
{
    Inifile ini;
    std::stringstream out;
    out <<
        #include "configs/autogen/str_ini.hpp"
    ;

    std::string contents = out.str();

    // remove comments
    auto end = std::remove_if(contents.begin(), contents.end(), [](char const& c){
        return c == '#' && (&c)[1] != ' ' && (&c)[1] != '_';
    });

    std::ofstream(wf.c_str()).write(contents.data(), end - contents.begin());
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));
}


RED_AUTO_TEST_CASE_WF(TestConfigFromFile, wf)
{
    // test we can read from a file (and not only from a stream)
    Inifile ini;
    ini.set<cfg::mod_rdp::disconnect_on_logon_user_change>(true);
    std::ofstream(wf.c_str()) <<
        "[mod_rdp]\n"
        "proxy_managed_drives = /tmp/raw/movie/\n"
        "disconnect_on_logon_user_change = no\n"
    ;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));
    RED_CHECK_EQUAL("/tmp/raw/movie/", ini.get<cfg::mod_rdp::proxy_managed_drives>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
}


RED_AUTO_TEST_CASE(TestConfigDefaultEmpty)
{
    // default config
    Inifile             ini;

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::high,                      ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                      ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,  ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900, ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,  ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120, ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600, ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/", ini.get<cfg::mod_replay::replay_path>());
    RED_CHECK_EQUAL(0440,    ini.get<cfg::audit::file_permissions>().permissions_as_uint());

    RED_CHECK_EQUAL("0.0.0.0",                   ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(false,                       ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("inquisition",               ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true, ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true, ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true, ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true, ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                memcmp(ini.get<cfg::crypto::encryption_key>().data(),
                                                               "\x00\x01\x02\x03\x04\x05\x06\x07"
                                                               "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
                                                               "\x10\x11\x12\x13\x14\x15\x16\x17"
                                                               "\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F", 32));
    RED_CHECK_EQUAL(0,                                memcmp(ini.get<cfg::crypto::sign_key>().data(),
                                                               "\x00\x01\x02\x03\x04\x05\x06\x07"
                                                               "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
                                                               "\x10\x11\x12\x13\x14\x15\x16\x17"
                                                               "\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F", 32));

    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false, ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(0x2c,  ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,     ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false, ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1, ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false, ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24, ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false, ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false, ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1, ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",   ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",    ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",    ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",   ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",    ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",    ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false, ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(40000, ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user, ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(5000,  ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",    ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_vnc::clipboard_up>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_vnc::clipboard_down>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1, ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed, ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,     ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(600,   ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24, ini.get<cfg::context::opt_bpp>());

    RED_CHECK_EQUAL(false, ini.is_asked<cfg::context::selector>());
    RED_CHECK_EQUAL(false, ini.is_asked<cfg::context::selector_current_page>());

    RED_CHECK_EQUAL(false, ini.get<cfg::context::selector>());
    RED_CHECK_EQUAL(1,     ini.get<cfg::context::selector_current_page>());
    RED_CHECK_EQUAL("",    ini.get<cfg::context::selector_device_filter>());
    RED_CHECK_EQUAL("",    ini.get<cfg::context::selector_group_filter>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::context::selector_lines_per_page>());
    RED_CHECK_EQUAL(1,     ini.get<cfg::context::selector_number_of_pages>());

    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::globals::target_device>());
    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::context::target_password>());
    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::context::target_port>());
    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::context::target_protocol>());
    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::globals::target_user>());

    RED_CHECK_EQUAL("",    ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(false, ini.is_asked<cfg::globals::host>());

    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::globals::auth_user>());
    RED_CHECK_EQUAL(true,  ini.is_asked<cfg::context::password>());

    RED_CHECK_EQUAL("",    ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",    ini.get<cfg::context::password>());

    RED_CHECK_EQUAL("",    ini.get<cfg::context::auth_channel_answer>());
    RED_CHECK_EQUAL("",    ini.get<cfg::context::auth_channel_target>());

    RED_CHECK_EQUAL("",    ini.get<cfg::context::message>());
    RED_CHECK_EQUAL(false, ini.get<cfg::context::accept_message>());
    RED_CHECK_EQUAL(false, ini.get<cfg::context::display_message>());

    RED_CHECK_EQUAL("",    ini.get<cfg::context::rejected>());


    RED_CHECK_EQUAL(false, ini.is_asked<cfg::context::keepalive>());

    RED_CHECK_EQUAL(false, ini.get<cfg::context::keepalive>());


    RED_CHECK_EQUAL("",    ini.get<cfg::context::session_id>());


    RED_CHECK_EQUAL(0,     ini.get<cfg::context::end_date_cnx>().count());

    RED_CHECK_EQUAL(RdpModeConsole::allow, ini.get<cfg::mod_rdp::mode_console>());

    RED_CHECK_EQUAL("",    ini.get<cfg::context::real_target_device>());

    RED_CHECK_EQUAL(false, ini.get<cfg::context::authentication_challenge>());
}


RED_AUTO_TEST_CASE_WF(TestConfig1, wf)
{
    // test we can read a config file with a global section
    std::ofstream(wf.c_str()) <<
        "[globals]\n"
        "port=3390\n"
        "listen_address=192.168.1.1\n"
        "enable_transparent_mode=yes\n"
        "certificate_password=redemption\n"
        "enable_bitmap_update=true\n"
        "enable_end_time_warning_osd=false\n"
        "enable_osd_display_remote_target=false\n"
        "authentication_timeout=150\n"
        "handshake_timeout=5\n"
        "\n"
        "[internal_mod]\n"
        "enable_close_box=false\n"
        "close_box_timeout=900\n"
        "\n"
        "[client]\n"
        "encryption_level=low\n"
        "ignore_logon_password=yes\n"
        "tls_fallback_legacy=yes\n"
        "tls_support=no\n"
        "rdp_compression=1\n"
        "max_color_depth=0\n" /* unknown value */
        "persistent_disk_bitmap_cache=yes\n"
        "cache_waiting_list=no\n"
        "persist_bitmap_cache_on_disk=yes\n"
        "bitmap_compression=true\n"
        "fast_path=true\n"
        "\n"
        "[mod_rdp]\n"
        "force_performance_flags=-wallpaper,\n"
        "disconnect_on_logon_user_change=yes\n"
        "enable_nla=yes\n"
        "open_session_timeout=45\n"
        "persistent_disk_bitmap_cache=false\n"
        "cache_waiting_list=no\n"
        "persist_bitmap_cache_on_disk=true\n"
        "allowed_channels=audin\n"
        "denied_channels=*\n"
        "fast_path=no\n"
        "proxy_managed_drives=\n"
        "alternate_shell=C:\\WINDOWS\\NOTEPAD.EXE\n"
        "shell_working_directory=C:\\WINDOWS\\\n"
        "[session_probe]\n"
        "enable_session_probe=true\n"
        "enable_launch_mask=false\n"
        "launch_timeout=0\n"
        "keepalive_timeout=0\n"
        "\n"
        "[mod_vnc]\n"
        "clipboard_up=yes\n"
        "encodings=16,2,0,1,-239\n"
        "server_clipboard_encoding_type=latin1\n"
        "bogus_clipboard_infinite_loop=0\n"
        "\n"
        "[capture]\n"
        "hash_path=/mnt/wab/hash\n"
        "record_path=/mnt/wab/recorded/rdp\n"
        "record_tmp_path=/mnt/tmp/wab/recorded/rdp\n"
        "disable_clipboard_log=1\n"
        "\n"
        "[debug]\n"
        "password=1\n"
        "compression=256\n"
        "cache=128\n"
        "[translation]\n"
        "\n"
    ;

    Inifile ini;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3390,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::low,                       ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                      ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL("/mnt/wab/hash/",                 ini.get<cfg::capture::hash_path>());
    RED_CHECK_EQUAL("/mnt/wab/recorded/rdp/",         ini.get<cfg::capture::record_path>());
    RED_CHECK_EQUAL("/mnt/tmp/wab/recorded/rdp/",     ini.get<cfg::capture::record_tmp_path>());

    RED_CHECK_EQUAL(ClipboardLogFlags::wrm,   ini.get<cfg::capture::disable_clipboard_log>());
    RED_CHECK_EQUAL(FileSystemLogFlags::none, ini.get<cfg::capture::disable_file_system_log>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16, ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip, ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(5,     ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,   ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,    ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(150,   ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(900,   ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",               ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("192.168.1.1",         ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(true,                  ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("redemption",          ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,  ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(false, ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(false, ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(false, ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(1,     ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(256,   ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(128,   ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(true,  ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(1,     ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,     ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(false, ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp4, ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(ColorDepth::depth24, ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false, ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1, ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(45,    ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("audin", ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("*",   ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",    ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",   ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("C:\\WINDOWS\\NOTEPAD.EXE", ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("C:\\WINDOWS\\", ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(true,  ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(false, ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user, ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(0,     ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL(true,  ini.get<cfg::mod_vnc::clipboard_up>());
    RED_CHECK_EQUAL(false, ini.get<cfg::mod_vnc::clipboard_down>());
    RED_CHECK_EQUAL("16,2,0,1,-239", ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1, ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed, ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,     ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,   ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,   ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24, ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE_WF(TestConfig1bis, wf)
{
    // test we can read a config file with a global section
    // alternative ways to say yes in file, other values
    std::ofstream(wf.c_str()) <<
        "[globals]\n"
        "listen_address=0.0.0.0\n"
        "enable_transparent_mode=no\n"
        "certificate_password=\n"
        "shell_working_directory=aaa\n" /* bad section */
        "enable_bitmap_update=no\n"
        "[client]\n"
        "encryption_level=medium\n"
        "tls_support=yes\n"
        "rdp_compression=0\n"
        "max_color_depth=8\n"
        "persistent_disk_bitmap_cache=no\n"
        "cache_waiting_list=yes\n"
        "bitmap_compression=on\n"
        "fast_path=yes\n"
        "[translation]\n"
        "[mod_rdp]\n"
        "force_performance_flags=1\n"
        "rdp_compression=2\n"
        "disconnect_on_logon_user_change=no\n"
        "enable_nla=no\n"
        "open_session_timeout=30\n"
        "persistent_disk_bitmap_cache=yes\n"
        "cache_waiting_list=no\n"
        "persist_bitmap_cache_on_disk=no\n"
        "fast_path=yes\n"
        "proxy_managed_drives=*docs\n"
        "alternate_shell=\n"
        "[session_probe]\n"
        "enable_session_probe=false\n"
        "launch_timeout=3000\n"
        "keepalive_timeout=6000\n"
        "[mod_replay]\n"
        "on_end_of_data=1\n"
        "[capture]\n"
        "hash_path=/mnt/wab/hash/\n"
        "record_path=/mnt/wab/recorded/rdp/\n"
        "record_tmp_path=/mnt/tmp/wab/recorded/rdp/\n"
        "disable_clipboard_log=1\n"
        "disable_file_system_log=2\n"
        "\n"
        "[mod_vnc]\n"
        "server_clipboard_encoding_type=utf-8\n"
        "bogus_clipboard_infinite_loop=1\n"
        "\n"
    ;

    Inifile ini;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::medium,                    ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                      ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL("/mnt/wab/hash/",                 ini.get<cfg::capture::hash_path>());
    RED_CHECK_EQUAL("/mnt/wab/recorded/rdp/",         ini.get<cfg::capture::record_path>());
    RED_CHECK_EQUAL("/mnt/tmp/wab/recorded/rdp/",     ini.get<cfg::capture::record_tmp_path>());

    RED_CHECK_EQUAL(ClipboardLogFlags::wrm,           ini.get<cfg::capture::disable_clipboard_log>());
    RED_CHECK_EQUAL(FileSystemLogFlags::meta,         ini.get<cfg::capture::disable_file_system_log>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("0.0.0.0",                        ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK(ini.get<cfg::crypto::encryption_key>() ==
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"_av);
    RED_CHECK(ini.get<cfg::crypto::sign_key>() ==
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"_av);

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(1,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::none,             ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth8,               ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp5,             ini.get<cfg::mod_rdp::rdp_compression>()); RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("*docs",                          ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(3000,                             ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(6000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::utf8,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::duplicated,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(1,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE_WF(TestConfig2, wf)
{
    // test we can read a config file with a global section, other values
    std::ofstream(wf.c_str()) <<
        "[globals]\n"
        "bitmap_cache=no\n"
        "listen_address=127.0.0.1\n"
        "certificate_password=rdpproxy\n"
        "enable_transparent_mode=true\n"
        "shell_working_directory=\n"
        "[client]\n"
        "encryption_level=low\n"
        "tls_support=yes\n"
        "max_color_depth=24\n"
        "persistent_disk_bitmap_cache=yes\n"
        "cache_waiting_list=no\n"
        "persist_bitmap_cache_on_disk=no\n"
        "bitmap_compression=false\n"
        "[mod_rdp]\n"
        "force_performance_flags=1\n"
        "rdp_compression=0\n"
        "proxy_managed_drives=*\n"
        "alternate_shell=C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98\\Bin\\MSDEV.EXE\n"
        "[session_probe]\n"
        "enable_session_probe=\n"
        "on_launch_failure=2\n"
        "[mod_replay]\n"
        "on_end_of_data=0\n"
        "[capture]\n"
        "wrm_color_depth_selection_strategy=1\n"
        "wrm_compression_algorithm=1\n"
        "\n"
    ;

    Inifile ini;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::low,                       ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("127.0.0.1",                      ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("rdpproxy",                       ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(1,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::none,             ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98\\Bin\\MSDEV.EXE",
                      ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(40000,                            ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::retry_without_session_probe,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(5000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE_WF(TestConfig3, wf)
{
    // test we can read a config file with a global section, other values
    std::ofstream(wf.c_str()) <<
        " [ globals ] \n"
        " bitmap_cache\t= no \n"
        "listen_address=127.0.0.1\n"
        "certificate_password=rdpproxy RDP\n"
        "enable_transparent_mode=true\n"
        "handshake_timeout=7\n"
        "[internal_mod]\n"
        "close_box_timeout=300\n"
        "[client]\t\n"
        "encryption_level=low\n"
        "tls_support=yes\n"
        "max_color_depth=24\n"
        "persistent_disk_bitmap_cache=yes\n"
        "cache_waiting_list=no\n"
        "persist_bitmap_cache_on_disk=no\n"
        "bitmap_compression=false\n"
        "\t[mod_rdp]\n"
        "force_performance_flags=1\n"
        "rdp_compression=0\n"
        "alternate_shell=C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98\\Bin\\MSDEV.EXE   \n"
        "shell_working_directory=\n"
        "[session_probe]\n"
        "launch_timeout=6000\n"
        "keepalive_timeout=3000\n"
        "[mod_replay]\n"
        "on_end_of_data=0\n"
        "[mod_vnc]\n"
        "bogus_clipboard_infinite_loop=2\n"
        "[capture]\n"
        "wrm_color_depth_selection_strategy=1\n"
        "wrm_compression_algorithm=1\n"
        "disable_clipboard_log=1\n"
        "\n"
    ;

    Inifile ini;

    ini.set<cfg::mod_rdp::shell_working_directory>("C:\\");

    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::low,                       ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ClipboardLogFlags::wrm,           ini.get<cfg::capture::disable_clipboard_log>());
    RED_CHECK_EQUAL(FileSystemLogFlags::none,         ini.get<cfg::capture::disable_file_system_log>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(7,                                ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(300,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("127.0.0.1",                      ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("rdpproxy RDP",                   ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(1,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::none,             ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98\\Bin\\MSDEV.EXE",
                      ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(6000,                             ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(3000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::continued,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE_WF(TestMultiple, wf)
{
    // test we can read a config file with a global section
    std::ofstream(wf.c_str()) <<
        "[globals]\n"
        "port=3390\n"
        "listen_address=0.0.0.0\n"
        "certificate_password=redemption\n"
        "enable_transparent_mode=False\n"
        "[client]\n"
        "encryption_level=high\n"
        "bitmap_compression=TRuE\n"
        "\n"
        "[mod_rdp]\n"
        "persistent_disk_bitmap_cache=true\n"
        "cache_waiting_list=no\n"
        "shell_working_directory=%HOMEDRIVE%%HOMEPATH%\n"
        "[session_probe]\n"
        "enable_session_probe=true\n"
        "\n"
    ;

    Inifile ini;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3390,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::high,                      ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("0.0.0.0",                        ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("redemption",                     ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(0x2c,                             ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("%HOMEDRIVE%%HOMEPATH%",          ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(40000,                            ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(5000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());


    // see we can change configuration using parse without default setting of existing ini
    std::ofstream(wf.c_str(), std::ios::trunc) <<
        "[globals]\n"
        "listen_address=192.168.1.1\n"
        "certificate_password=\n"
        "enable_transparent_mode=yes\n"
        "[client]\n"
        "bitmap_compression=no\n"
        "persist_bitmap_cache_on_disk=yes\n"
        "[mod_rdp]\n"
        "persist_bitmap_cache_on_disk=yes\n"
        "proxy_managed_drives=docs,apps\n"
        "[session_probe]\n"
        "launch_timeout=4000\n"
        "on_launch_failure=0\n"
        "keepalive_timeout=7000\n"
        "[mod_vnc]\n"
        "bogus_clipboard_infinite_loop=0\n"
        "[debug]\n"
        "password=3\n"
        "compression=0x3\n"
        "cache=0\n"
    ;
    RED_CHECK(configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3390,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::high,                      ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("192.168.1.1",                    ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(3,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(3,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(0x2c,                              ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("docs,apps",                      ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("%HOMEDRIVE%%HOMEPATH%",          ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(4000,                             ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::ignore_and_continue,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(7000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE_WF(TestNewConf, wf)
{
    // new behavior:
    // init() load default values from main configuration file
    // - options with multiple occurences get the last value
    // - unrecognized lines are ignored
    // - every characters following # are ignored until end of line (comments)
    Inifile             ini;

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());


    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::high,                      ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(120,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("0.0.0.0",                        ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("inquisition",                    ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(0x2c,                             ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(40000,                            ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(5000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());

    std::ofstream(wf.c_str()) <<
        "# Here we put global values\n"
        "[globals]\n"
        "# below we have lines with syntax errors, but they are just ignored\n"
        "authentication_timeout=300\n"
        "yyy\n"
        "zzz\n"
        "# unknwon keys are also ignored\n"
        "yyy=1\n"
        "[client]\n"
        "bitmap_compression=no\n"
    ;

    RED_CHECK(!configuration_load(Inifile::ConfigurationHolder(ini).as_ref(), wf.c_str()));

    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::is_rec>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::auth_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::host>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL(3389,                             ini.get<cfg::globals::port>());
    RED_CHECK_EQUAL(RdpSecurityEncryptionLevel::high,                      ini.get<cfg::client::encryption_level>());
    RED_CHECK_EQUAL("/tmp/redemption-sesman-sock",    ini.get<cfg::globals::authfile>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::audit::video_notimestamp>());

    RED_CHECK_EQUAL((CaptureFlags::png | CaptureFlags::wrm | CaptureFlags::ocr),
                                                        ini.get<cfg::capture::capture_flags>());
    RED_CHECK_EQUAL(1000,                             ini.get<cfg::audit::rt_png_interval>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::capture::wrm_break_interval>().count());

    RED_CHECK_EQUAL(5,                                ini.get<cfg::audit::rt_png_limit>());

    RED_CHECK_EQUAL(ColorDepthSelectionStrategy::depth16,
                                                        ini.get<cfg::capture::wrm_color_depth_selection_strategy>());
    RED_CHECK_EQUAL(WrmCompressionAlgorithm::gzip,
                                                        ini.get<cfg::capture::wrm_compression_algorithm>());

    RED_CHECK_EQUAL(10,                               ini.get<cfg::globals::handshake_timeout>().count());
    RED_CHECK_EQUAL(900,                              ini.get<cfg::globals::base_inactivity_timeout>().count());
    RED_CHECK_EQUAL(30,                               ini.get<cfg::globals::keepalive_grace_delay>().count());
    RED_CHECK_EQUAL(300,                              ini.get<cfg::globals::authentication_timeout>().count());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::internal_mod::close_box_timeout>().count());

    RED_CHECK_EQUAL("/tmp/",                          ini.get<cfg::mod_replay::replay_path>());

    RED_CHECK_EQUAL("0.0.0.0",                        ini.get<cfg::globals::listen_address>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::globals::enable_transparent_mode>());
    RED_CHECK_EQUAL("inquisition",                    ini.get<cfg::globals::certificate_password>());

    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_bitmap_update>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::internal_mod::enable_close_box>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_end_time_warning_osd>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::globals::enable_osd_display_remote_target>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::capture>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::auth>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::session>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::front>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_rdp>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_vnc>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::mod_internal>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::password>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::compression>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::debug::cache>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::ignore_logon_password>());
    RED_CHECK_EQUAL(0x2c,                             ini.get<cfg::mod_rdp::force_performance_flags>().force_present);
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::force_performance_flags>().force_not_present);
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::tls_support>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::tls_fallback_legacy>());
    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::client::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::disable_tsk_switch_shortcuts>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::client::max_color_depth>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::client::bitmap_compression>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::client::fast_path>());

    RED_CHECK_EQUAL(RdpCompression::rdp6_1,           ini.get<cfg::mod_rdp::rdp_compression>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::disconnect_on_logon_user_change>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::enable_nla>());
    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_rdp::open_session_timeout>().count());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::persistent_disk_bitmap_cache>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::cache_waiting_list>());
    RED_CHECK_EQUAL(false,                            ini.get<cfg::mod_rdp::persist_bitmap_cache_on_disk>());
    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::allowed_channels>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::denied_channels>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::mod_rdp::fast_path>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::proxy_managed_drives>());

    RED_CHECK_EQUAL("*",                              ini.get<cfg::mod_rdp::auth_channel>());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::alternate_shell>());
    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_rdp::shell_working_directory>());

    RED_CHECK_EQUAL(false,                            ini.get<cfg::session_probe::enable_session_probe>());
    RED_CHECK_EQUAL(true,                             ini.get<cfg::session_probe::enable_launch_mask>());
    RED_CHECK_EQUAL(40000,                            ini.get<cfg::session_probe::launch_timeout>().count());
    RED_CHECK_EQUAL(SessionProbeOnLaunchFailure::disconnect_user,
                                                        ini.get<cfg::session_probe::on_launch_failure>());
    RED_CHECK_EQUAL(5000,                             ini.get<cfg::session_probe::keepalive_timeout>().count());

    RED_CHECK_EQUAL("",                               ini.get<cfg::mod_vnc::encodings>());
    RED_CHECK_EQUAL(ClipboardEncodingType::latin1,          ini.get<cfg::mod_vnc::server_clipboard_encoding_type>());
    RED_CHECK_EQUAL(VncBogusClipboardInfiniteLoop::delayed,
                                                        ini.get<cfg::mod_vnc::bogus_clipboard_infinite_loop>());

    RED_CHECK_EQUAL(0,                                ini.get<cfg::mod_replay::on_end_of_data>());

    RED_CHECK_EQUAL(800,                              ini.get<cfg::context::opt_width>());
    RED_CHECK_EQUAL(600,                              ini.get<cfg::context::opt_height>());
    RED_CHECK_EQUAL(ColorDepth::depth24,              ini.get<cfg::context::opt_bpp>());
}

RED_AUTO_TEST_CASE(TestLogPolicy)
{
    Inifile ini;
    using Cat = Inifile::LoggableCategory;

    RED_CHECK(Cat::Loggable ==
        get_acl_field(ini, cfg::globals::auth_user::index).loggable_category());
    RED_CHECK(Cat::Unloggable ==
        get_acl_field(ini, cfg::globals::target_application_password::index).loggable_category());
    RED_CHECK(Cat::LoggableButWithPassword ==
        get_acl_field(ini, cfg::context::auth_channel_answer::index).loggable_category());
}

RED_AUTO_TEST_CASE(TestFieldName)
{
    Inifile ini;

    RED_CHECK(not bool(ini.get_acl_field_by_name("Unknown"_av)));
    RED_CHECK("target_login"_av ==
        get_acl_field(ini, cfg::globals::target_user::index).get_acl_name());
    RED_CHECK("width"_av ==
        get_acl_field(ini, cfg::context::opt_width::index).get_acl_name());
    RED_CHECK("alternate_shell"_av ==
        get_acl_field(ini, cfg::mod_rdp::alternate_shell::index).get_acl_name());
    RED_CHECK("mod_rdp:enable_nla"_av ==
        get_acl_field(ini, cfg::mod_rdp::enable_nla::index).get_acl_name());
}

RED_AUTO_TEST_CASE(TestContextSetValue)
{
    Inifile ini;
    Inifile::ZStringBuffer zstring_buffer;

    auto get_zstring = [&](configs::authid_t id){
        auto zstr = get_acl_field(ini, id).to_zstring_view(zstring_buffer);
        RED_CHECK(zstr.data()[zstr.size()] == '\0');
        return zstr;
    };

    // selector, ...
    get_acl_field(ini, cfg::context::selector::index).ask();
    get_acl_field(ini, cfg::context::selector_current_page::index).ask();
    get_acl_field(ini, cfg::context::selector_device_filter::index).ask();
    get_acl_field(ini, cfg::context::selector_group_filter::index).ask();
    get_acl_field(ini, cfg::context::selector_lines_per_page::index).ask();

    RED_CHECK_EQUAL(true, ini.is_asked<cfg::context::selector>());
    RED_CHECK_EQUAL(true, ini.is_asked<cfg::context::selector_current_page>());

    get_acl_field(ini, cfg::context::selector::index).parse("True"_zv);
    get_acl_field(ini, cfg::context::selector_current_page::index).parse("2"_zv);
    get_acl_field(ini, cfg::context::selector_device_filter::index).parse("Windows"_zv);
    get_acl_field(ini, cfg::context::selector_group_filter::index).parse("RDP"_zv);
    get_acl_field(ini, cfg::context::selector_lines_per_page::index).parse("25"_zv);
    get_acl_field(ini, cfg::context::selector_number_of_pages::index).parse("2"_zv);

    RED_CHECK_EQUAL(false,     ini.is_asked<cfg::context::selector>());
    RED_CHECK_EQUAL(false,     ini.is_asked<cfg::context::selector_current_page>());

    RED_CHECK_EQUAL(true,      ini.get<cfg::context::selector>());
    RED_CHECK_EQUAL(2,         ini.get<cfg::context::selector_current_page>());
    RED_CHECK_EQUAL("Windows", ini.get<cfg::context::selector_device_filter>());
    RED_CHECK_EQUAL("RDP",     ini.get<cfg::context::selector_group_filter>());
    RED_CHECK_EQUAL(25,        ini.get<cfg::context::selector_lines_per_page>());
    RED_CHECK_EQUAL(2,         ini.get<cfg::context::selector_number_of_pages>());

    RED_CHECK_EQUAL("True"_av,    get_zstring(cfg::context::selector::index));
    RED_CHECK_EQUAL("2"_av,       get_zstring(cfg::context::selector_current_page::index));
    RED_CHECK_EQUAL("Windows"_av, get_zstring(cfg::context::selector_device_filter::index));
    RED_CHECK_EQUAL("RDP"_av,     get_zstring(cfg::context::selector_group_filter::index));
    RED_CHECK_EQUAL("25"_av,      get_zstring(cfg::context::selector_lines_per_page::index));
    RED_CHECK_EQUAL("2"_av,       get_zstring(cfg::context::selector_number_of_pages::index));


    // target_xxxx
    get_acl_field(ini, cfg::globals::target_device::index).parse("127.0.0.1"_zv);
    get_acl_field(ini, cfg::context::target_password::index).parse("12345678"_zv);
    get_acl_field(ini, cfg::context::target_port::index).parse("3390"_zv);
    get_acl_field(ini, cfg::context::target_protocol::index).parse("RDP"_zv);
    get_acl_field(ini, cfg::globals::target_user::index).parse("admin"_zv);
    get_acl_field(ini, cfg::globals::target_application::index).parse("wallix@putty"_zv);

    RED_CHECK_EQUAL(false,          ini.is_asked<cfg::globals::target_device>());
    RED_CHECK_EQUAL(false,          ini.is_asked<cfg::context::target_password>());
    RED_CHECK_EQUAL(false,          ini.is_asked<cfg::context::target_port>());
    RED_CHECK_EQUAL(false,          ini.is_asked<cfg::context::target_protocol>());
    RED_CHECK_EQUAL(false,          ini.is_asked<cfg::globals::target_user>());

    RED_CHECK_EQUAL("127.0.0.1",    ini.get<cfg::globals::target_device>());
    RED_CHECK_EQUAL("12345678",     ini.get<cfg::context::target_password>());
    RED_CHECK_EQUAL(3390,           ini.get<cfg::context::target_port>());
    RED_CHECK_EQUAL("RDP",          ini.get<cfg::context::target_protocol>());
    RED_CHECK_EQUAL("admin",        ini.get<cfg::globals::target_user>());
    RED_CHECK_EQUAL("wallix@putty", ini.get<cfg::globals::target_application>());

    RED_CHECK_EQUAL("127.0.0.1"_av,    get_zstring(cfg::globals::target_device::index));
    RED_CHECK_EQUAL("12345678"_av,     get_zstring(cfg::context::target_password::index));
    RED_CHECK_EQUAL("3390"_av,         get_zstring(cfg::context::target_port::index));
    RED_CHECK_EQUAL("RDP"_av,          get_zstring(cfg::context::target_protocol::index));
    RED_CHECK_EQUAL("admin"_av,        get_zstring(cfg::globals::target_user::index));
    RED_CHECK_EQUAL("wallix@putty"_av, get_zstring(cfg::globals::target_application::index));


    // host
    get_acl_field(ini, cfg::globals::host::index).ask();

    RED_CHECK_EQUAL(true, ini.is_asked<cfg::globals::host>());

    get_acl_field(ini, cfg::globals::host::index).parse("127.0.0.1"_zv);

    RED_CHECK_EQUAL(false,       ini.is_asked<cfg::globals::host>());

    RED_CHECK_EQUAL("127.0.0.1", ini.get<cfg::globals::host>());

    RED_CHECK_EQUAL("127.0.0.1"_av, get_zstring(cfg::globals::host::index));

    // auth_user
    get_acl_field(ini, cfg::globals::auth_user::index).ask();

    RED_CHECK_EQUAL(true, ini.is_asked<cfg::globals::auth_user>());

    get_acl_field(ini, cfg::globals::auth_user::index).parse("192.168.0.1"_zv);

    RED_CHECK_EQUAL(false,         ini.is_asked<cfg::globals::auth_user>());

    RED_CHECK_EQUAL("192.168.0.1", ini.get<cfg::globals::auth_user>());

    RED_CHECK_EQUAL("192.168.0.1"_av, get_zstring(cfg::globals::auth_user::index));

    // password
    get_acl_field(ini, cfg::context::password::index).ask();

    RED_CHECK_EQUAL(true, ini.is_asked<cfg::context::password>());

    get_acl_field(ini, cfg::context::password::index).parse("12345678"_zv);

    RED_CHECK_EQUAL(false,      ini.is_asked<cfg::context::password>());

    RED_CHECK_EQUAL("12345678", ini.get<cfg::context::password>());

    RED_CHECK_EQUAL("12345678"_av, get_zstring(cfg::context::password::index));


    // answer
    get_acl_field(ini, cfg::context::auth_channel_answer::index).parse("answer"_zv);

    RED_CHECK_EQUAL("answer", ini.get<cfg::context::auth_channel_answer>());


    // message
    get_acl_field(ini, cfg::context::message::index).parse("message"_zv);

    RED_CHECK_EQUAL("message", ini.get<cfg::context::message>());


    // rejected
    get_acl_field(ini, cfg::context::rejected::index).parse("rejected"_zv);

    RED_CHECK_EQUAL("rejected", ini.get<cfg::context::rejected>());

    RED_CHECK_EQUAL("rejected"_av, get_zstring(cfg::context::rejected::index));


    // keepalive
    get_acl_field(ini, cfg::context::keepalive::index).parse("True"_zv);

    RED_CHECK_EQUAL(false, ini.is_asked<cfg::context::keepalive>());

    RED_CHECK_EQUAL(true,  ini.get<cfg::context::keepalive>());


    // session_id
    get_acl_field(ini, cfg::context::session_id::index).parse("0123456789"_zv);

    RED_CHECK_EQUAL("0123456789", ini.get<cfg::context::session_id>());


    // end_date_cnx
    get_acl_field(ini, cfg::context::end_date_cnx::index).parse("12345678"_zv);

    RED_CHECK_EQUAL(12345678, ini.get<cfg::context::end_date_cnx>().count());

    // mode_console
    get_acl_field(ini, cfg::mod_rdp::mode_console::index).parse("forbid"_zv);

    RED_CHECK_EQUAL(RdpModeConsole::forbid, ini.get<cfg::mod_rdp::mode_console>());

    // real_target_device
    get_acl_field(ini, cfg::context::real_target_device::index).parse("10.0.0.1"_zv);

    RED_CHECK_EQUAL("10.0.0.1", ini.get<cfg::context::real_target_device>());

    RED_CHECK_EQUAL("10.0.0.1"_av, get_zstring(cfg::context::real_target_device::index));


    // authentication_challenge
    get_acl_field(ini, cfg::context::authentication_challenge::index).parse("true"_zv);

    RED_CHECK_EQUAL(true, ini.get<cfg::context::authentication_challenge>());
}

RED_AUTO_TEST_CASE(TestConfigSetAndUpdate)
{
    Inifile ini;

    cfg::crypto::encryption_key::type akey{{}};
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() != akey);

    unsigned char ckey[32]{1};
    ini.set<cfg::crypto::encryption_key>(ckey);
    akey[0] = 1;
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() == akey);

    unsigned char const cckey[32]{1, 1};
    ini.set<cfg::crypto::encryption_key>(cckey);
    akey[1] = 1;
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() == akey);

    akey[2] = 1;
    ini.set<cfg::crypto::encryption_key>(akey);
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() == akey);

    ini.update<cfg::crypto::encryption_key>([](auto& key){
        key[3] = 1;
    });
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() != akey);
    akey[3] = 1;
    RED_CHECK(ini.get<cfg::crypto::encryption_key>() == akey);
}

RED_AUTO_TEST_CASE(TestConfigNotifications)
{
    Inifile ini;

    ini.clear_acl_fields_changed();
    RED_CHECK(ini.get_acl_fields_changed().size() == 0);

    // auth_user has been changed, so check_from_acl() method will notify that something changed
    RED_CHECK(get_acl_field(ini, cfg::globals::auth_user::index).parse("someoneelse"_zv));
    RED_CHECK_EQUAL("someoneelse", ini.get<cfg::globals::auth_user>());

    ini.clear_acl_fields_changed();
    RED_CHECK(ini.get_acl_fields_changed().size() == 0);

    // setting a field without changing it should not notify that something changed
    ini.set_acl<cfg::globals::auth_user>("someoneelse");
    RED_CHECK(ini.get_acl_fields_changed().size() == 1);

    // Using the list of changed fields:
    ini.set_acl<cfg::globals::auth_user>("someuser");
    ini.set_acl<cfg::globals::host>("35.53.0.1");
    ini.set_acl<cfg::context::opt_height>(602u);
    ini.update_acl<cfg::globals::target>([](std::string& s){
        s = "35.53.0.2";
    });

    auto list = ini.get_acl_fields_changed();
    RED_CHECK_EQUAL(4, list.size());

    for (auto var : list) {
        RED_CHECK((
            var.authid() == cfg::globals::auth_user::index
         || var.authid() == cfg::globals::host::index
         || var.authid() == cfg::context::opt_height::index
         || var.authid() == cfg::globals::target::index
        ));
    }
    ini.clear_acl_fields_changed();
    RED_CHECK(ini.get_acl_fields_changed().size() == 0);

    RED_CHECK(ini.get<cfg::globals::auth_user>() == "someuser"_av);
    RED_CHECK(ini.get<cfg::globals::host>() == "35.53.0.1"_av);
    RED_CHECK(ini.get<cfg::context::opt_height>() == 602u);
    RED_CHECK(ini.get<cfg::globals::target>() == "35.53.0.2"_av);
}

RED_AUTO_TEST_CASE(TestConfigThemeDefault)
{
    Theme theme;
    Inifile ini;

    auto bgr = [](configs::spec_types::rgb rgb){
        return BGRColor(BGRasRGBColor(BGRColor(rgb.to_rgb888())));
    };

    RED_CHECK(bgr(ini.get<cfg::theme::bgcolor>()) == theme.global.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::fgcolor>()) == theme.global.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::separator_color>()) == theme.global.separator_color);
    RED_CHECK(bgr(ini.get<cfg::theme::focus_color>()) == theme.global.focus_color);
    RED_CHECK(bgr(ini.get<cfg::theme::error_color>()) == theme.global.error_color);
    RED_CHECK(bgr(ini.get<cfg::theme::edit_bgcolor>()) == theme.edit.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::edit_fgcolor>()) == theme.edit.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::edit_border_color>()) == theme.edit.border_color);
    RED_CHECK(bgr(ini.get<cfg::theme::tooltip_bgcolor>()) == theme.tooltip.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::tooltip_fgcolor>()) == theme.tooltip.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::tooltip_border_color>()) == theme.tooltip.border_color);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_line1_bgcolor>()) == theme.selector_line1.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_line1_fgcolor>()) == theme.selector_line1.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_line2_bgcolor>()) == theme.selector_line2.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_line2_fgcolor>()) == theme.selector_line2.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_focus_bgcolor>()) == theme.selector_focus.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_focus_fgcolor>()) == theme.selector_focus.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_selected_bgcolor>()) == theme.selector_selected.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_selected_fgcolor>()) == theme.selector_selected.fgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_label_bgcolor>()) == theme.selector_label.bgcolor);
    RED_CHECK(bgr(ini.get<cfg::theme::selector_label_fgcolor>()) == theme.selector_label.fgcolor);
}
