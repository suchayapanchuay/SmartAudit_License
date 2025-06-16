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
   Copyright (C) Wallix 2010-2013
   Author(s): Clément Moroldo, David Fort
*/

#include "client_redemption/client_config/client_redemption_config.hpp"

#include "capture/cryptofile.hpp"
#include "core/RDP/clipboard.hpp"
#include "core/RDP/channels/rdpdr.hpp"
#include "transport/crypto_transport.hpp"
#include "transport/mwrm_reader.hpp"
#include "utils/cli.hpp"
#include "utils/sugar/unique_fd.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "utils/fileutils.hpp"
#include "utils/redemption_info_version.hpp"

#include <iostream>


ClientRedemptionConfig::ClientRedemptionConfig(RDPVerbose verbose, const std::string &MAIN_DIR )
: MAIN_DIR((MAIN_DIR.empty() || MAIN_DIR == "/")
            ? MAIN_DIR
            : (MAIN_DIR.back() == '/')
            ? MAIN_DIR.substr(0, MAIN_DIR.size() - 1)
            : MAIN_DIR)
, verbose(verbose) {}



time_t ClientConfig::get_movie_time_length(const char * mwrm_filename) {
    // TODO RZ: Support encrypted recorded file.

    CryptoContext cctx;
    InCryptoTransport trans(cctx, InCryptoTransport::EncryptionMode::NotEncrypted);
    MwrmReader mwrm_reader(trans);
    MetaLine meta_line;

    std::chrono::seconds start_time {};
    std::chrono::seconds stop_time {};

    trans.open(mwrm_filename);
    mwrm_reader.read_meta_headers();

    Transport::Read read_stat = mwrm_reader.read_meta_line(meta_line);

    if (read_stat == Transport::Read::Ok) {
        start_time = meta_line.start_time;
        stop_time = meta_line.stop_time;
        while (read_stat == Transport::Read::Ok) {
            stop_time = meta_line.stop_time;
            read_stat = mwrm_reader.read_meta_line(meta_line);
        }
    }

    return (stop_time - start_time).count();
}

void ClientConfig::openWindowsData(ClientRedemptionConfig & config)  {
    unique_fd file = unique_fd(config.WINDOWS_CONF.c_str(), O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);

    if(file.is_open()) {
        config.windowsData.no_data = false;
        std::array<char, 4096> buffer;
        auto len = ::read(file.fd(), buffer.data(), buffer.size() - 1);
        if (len > 0) {
            buffer[std::size_t(len)] = 0;
            char* s = buffer.data();
            for (int* n : {
                &config.windowsData.form_x,
                &config.windowsData.form_y,
                &config.windowsData.screen_x,
                &config.windowsData.screen_y,
            }) {
                if (!(s = std::strchr(s, ' '))) return;
                *n = std::strtol(s, &s, 10);
            }
        }
    }
}

void ClientConfig::writeWindowsData(WindowsData & config)
{
    unique_fd fd(config.config_file_path.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd.is_open()) {
        std::string info = str_concat(
            "form_x ", int_to_decimal_chars(config.form_x), '\n',
            "form_y ", int_to_decimal_chars(config.form_y), '\n',
            "screen_x ", int_to_decimal_chars(config.screen_x), '\n',
            "screen_y ", int_to_decimal_chars(config.screen_y), '\n');

        ::write(fd.fd(), info.c_str(), info.length());
    }
}

void ClientConfig::parse_options(int argc, char const* const argv[], ClientRedemptionConfig & config)
{
    config.info.screen_info.width = 800;
    config.info.screen_info.height = 600;

     auto options = cli::options(
        cli::helper("Client ReDemPtion Help menu."),

        cli::option('h', "help").help("Show help")
        .parser(cli::help()),

        cli::option('v', "version").help("Show version")
        .parser(cli::quit([]{ std::cout << redemption_info_version() << "\n"; })),

        cli::helper("========= Connection ========="),

        cli::option('u', "username").help("Set target session user name")
        .parser(cli::arg([&config](std::string s) {
            config.user_name = std::move(s);
            config.connection_info_cmd_complete |= ClientRedemptionConfig::NAME_GOT;
        })),

        cli::option('p', "password").help("Set target session user password")
        .parser(cli::arg([&config](std::string s) {
            config.user_password = std::move(s);
            config.connection_info_cmd_complete |= ClientRedemptionConfig::PWD_GOT;
        })),

        cli::option('i', "ip").help("Set target IP address")
        .parser(cli::arg([&config](std::string s) {
            config.target_IP = std::move(s);
            config.connection_info_cmd_complete |= ClientRedemptionConfig::IP_GOT;
        })),

        cli::option('P', "port").help("Set port to use on target")
        .parser(cli::arg([&config](int n) {
            config.port = n;
            config.connection_info_cmd_complete |= ClientRedemptionConfig::PORT_GOT;
        })),

        cli::helper("========= Verbose ========="),

        cli::option("rdpdr").help("Active rdpdr logs")
        .parser(cli::on_off_bit_location<RDPVerbose::rdpdr>(config.verbose)),

        cli::option("rdpsnd").help("Active rdpsnd logs")
        .parser(cli::on_off_bit_location<RDPVerbose::rdpsnd>(config.verbose)),

        cli::option("cliprdr").help("Active cliprdr logs")
        .parser(cli::on_off_bit_location<RDPVerbose::cliprdr>(config.verbose)),

        cli::option("graphics").help("Active graphics logs")
        .parser(cli::on_off_bit_location<RDPVerbose::graphics>(config.verbose)),

        cli::option("printer").help("Active printer logs")
        .parser(cli::on_off_bit_location<RDPVerbose::printer>(config.verbose)),

        cli::option("rdpdr-dump").help("Actives rdpdr logs and dump brute rdpdr PDU")
        .parser(cli::on_off_bit_location<RDPVerbose::rdpdr_dump>(config.verbose)),

        cli::option("cliprd-dump").help("Actives cliprdr logs and dump brute cliprdr PDU")
        .parser(cli::on_off_bit_location<RDPVerbose::cliprdr_dump>(config.verbose)),

        cli::option("basic-trace").help("Active basic-trace logs")
        .parser(cli::on_off_bit_location<RDPVerbose::basic_trace>(config.verbose)),

        cli::option("connection").help("Active connection logs")
        .parser(cli::on_off_bit_location<RDPVerbose::connection>(config.verbose)),

        cli::option("rail-order").help("Active rail-order logs")
        .parser(cli::on_off_bit_location<RDPVerbose::rail_order>(config.verbose)),

        cli::option("asynchronous-task").help("Active asynchronous-task logs")
        .parser(cli::on_off_bit_location<RDPVerbose::asynchronous_task>(config.verbose)),

        cli::option("capabilities").help("Active capabilities logs")
        .parser(cli::on_off_bit_location<RDPVerbose::capabilities>(config.verbose)),

        cli::option("rail").help("Active rail logs")
        .parser(cli::on_off_bit_location<RDPVerbose::rail>(config.verbose)),

        cli::option("rail-dump").help("Actives rail logs and dump brute rail PDU")
        .parser(cli::on_off_bit_location<RDPVerbose::rail_dump>(config.verbose)),


        cli::helper("========= Protocol ========="),

        cli::option("vnc").help("Set connection mod to VNC")
        .parser(cli::trigger([&config]() {
            config.mod_state = ClientRedemptionConfig::MOD_VNC;
            if (!bool(config.connection_info_cmd_complete & ClientRedemptionConfig::PORT_GOT)) {
                config.port = 5900;
            }
        })),

        cli::option("rdp").help("Set connection mod to RDP (default).")
        .parser(cli::trigger([&config]() {
            config.mod_state = ClientRedemptionConfig::MOD_RDP;
            config.port = 3389;
        })),

        cli::option("remote-app").help("Connection as remote application.")
        .parser(cli::on_off_bit_location<ClientRedemptionConfig::MOD_RDP_REMOTE_APP>(config.mod_state)),

        cli::option("remote-exe").help("Connection as remote application and set the line command.")
        .argname("command")
        .parser(cli::arg([&config](std::string_view line) {
            config.mod_state = ClientRedemptionConfig::MOD_RDP_REMOTE_APP;
            config.modRDPParamsData.enable_shared_remoteapp = true;
            auto pos(line.find(' '));
            if (pos == std::string::npos) {
                config.rDPRemoteAppConfig.source_of_ExeOrFile = line;
                config.rDPRemoteAppConfig.source_of_Arguments.clear();
            }
            else {
                config.rDPRemoteAppConfig.source_of_ExeOrFile = line.substr(0, pos);
                config.rDPRemoteAppConfig.source_of_Arguments = line.substr(pos + 1);
            }
        })),

        cli::option("span").help("Span the screen size on local screen")
        .parser(cli::on_off_location(config.is_spanning)),

        cli::option("enable-clipboard").help("Enable clipboard sharing")
        .parser(cli::on_off_location(config.enable_shared_clipboard)),

        cli::option("enable-nla").help("Enable NLA protocol")
        .parser(cli::on_off_location(config.modRDPParamsData.enable_nla)),

        cli::option("enable-tls").help("Enable TLS protocol")
        .parser(cli::on_off_location(config.modRDPParamsData.enable_tls)),

        cli::option("tls-min-level").help("Minimal TLS protocol level")
        .parser(cli::arg_location(config.tls_client_params_data.tls_min_level)),

        cli::option("tls-max-level").help("Maximal TLS protocol level allowed")
        .parser(cli::arg_location(config.tls_client_params_data.tls_max_level)),

        cli::option("show_common_cipher_list").help("Show TLS Cipher List")
        .parser(cli::on_off_location(config.tls_client_params_data.show_common_cipher_list)),

        cli::option("cipher_string").help("Set TLS Cipher allowed for TLS <= 1.2")
        .parser(cli::arg([&config](std::string s){
            config.tls_client_params_data.cipher_string = std::move(s);
        })),

        cli::option("enable-sound").help("Enable sound")
        .parser(cli::on_off_location(config.modRDPParamsData.enable_sound)),

        cli::option("enable-fullwindowdrag").help("Enable full window draging")
        .parser(cli::on_off_bit_location<~PERF_DISABLE_FULLWINDOWDRAG>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-menuanimations").help("Enable menu animations")
        .parser(cli::on_off_bit_location<~PERF_DISABLE_MENUANIMATIONS>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-theming").help("Enable theming")
        .parser(cli::on_off_bit_location<~PERF_DISABLE_THEMING>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-cursor-shadow").help("Enable cursor shadow")
        .parser(cli::on_off_bit_location<~PERF_DISABLE_CURSOR_SHADOW>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-cursorsettings").help("Enable cursor settings")
        .parser(cli::on_off_bit_location<~PERF_DISABLE_CURSORSETTINGS>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-font-smoothing").help("Enable font smoothing")
        .parser(cli::on_off_bit_location<PERF_ENABLE_FONT_SMOOTHING>(
            config.info.rdp5_performanceflags)),

        cli::option("enable-desktop-composition").help("Enable desktop composition")
        .parser(cli::on_off_bit_location<PERF_ENABLE_DESKTOP_COMPOSITION>(
            config.info.rdp5_performanceflags)),

        cli::option("vnc-applekeyboard").help("Set keyboard compatibility mod with apple VNC server")
        .parser(cli::on_off_location(config.modVNCParamsData.is_apple)),

        cli::option("keep_alive_frequence")
        .help("Set timeout to send keypress to keep the session alive")
        .parser(cli::arg([&](int t){ config.keep_alive_freq = t; })),

        cli::option("remotefx").help("enable remotefx")
        .parser(cli::on_off_location(config.enable_remotefx)),


        cli::helper("========= Client ========="),

        cli::option("width").help("Set screen width")
        .parser(cli::arg_location(config.info.screen_info.width)),

        cli::option("height").help("Set screen height")
        .parser(cli::arg_location(config.info.screen_info.height)),

        cli::option("bpp").help("Set bit per pixel (8, 15, 16, 24, 32)")
        .argname("bit_per_pixel")
        .parser(cli::arg([&config](int x) {
            config.info.screen_info.bpp = checked_int(x);
        })),

        cli::option("keylayout").help("Set windows keylayout")
        .parser(cli::arg_location(config.info.keylayout)),

        cli::option("enable-record").help("Enable session recording as .wrm movie")
        .parser(cli::on_off_location(config.is_recording)),

        cli::option("persist").help("Set connection to persist")
        .parser(cli::trigger([&config]() {
            config.quick_connection_test = false;
            config.persist = true;
        })),

        cli::option("timeout").help("Set timeout response before to disconnect in millisecond")
        .argname("time")
        .parser(cli::arg([&](long time){
            config.quick_connection_test = false;
            config.time_out_disconnection = std::chrono::milliseconds(time);
        })),

        cli::option("share-dir").help("Set directory path on local disk to share with your session.")
        .argname("directory")
        .parser(cli::arg([&config](std::string s) {
            config.modRDPParamsData.enable_shared_virtual_disk = !s.empty();
            config.SHARE_DIR = std::move(s);
        })),

        cli::option("remote-dir").help("Remote working directory")
        .argname("directory")
        .parser(cli::arg_location(config.rDPRemoteAppConfig.source_of_WorkingDir))
    );

    auto cli_result = cli::parse(options, argc, argv);
    switch (cli_result.res) {
        case cli::Res::Ok:
            break;
        case cli::Res::Exit:
            // TODO return 0;
            break;
        case cli::Res::Help:
            config.help_mode = true;
            cli::print_help(options, std::cout);
            // TODO return 0;
            break;
        case cli::Res::BadValueFormat:
        case cli::Res::MissingValue:
        case cli::Res::UnknownOption:
        case cli::Res::NotValueWithValue:
        case cli::Res::NotOption:
        case cli::Res::StopParsing:
            cli::print_error(cli_result, std::cerr);
            std::cerr << "\n";
            // TODO return 1;
            break;
    }
    if (bool(RDPVerbose::rail & config.verbose)) {
        config.verbose = config.verbose | RDPVerbose::rail_order;
    }
}


// TODO PERF very inneficient. replace to append_file_contents()
bool ClientConfig::read_line(const int fd, std::string & line) {
    line.clear();
    if (fd < 0) {
        return false;
    }
    char c[2] = {'\0', '\0'};
    int size = -1;
    while (c[0] != '\n' && size !=  0) {
        size_t size = ::read(fd, c, 1);
        if (size == 1) {
            if (c[0] == '\n') {
                return true;
            }
            line += c[0];
        }
        else {
            return false;
        }
    }
    return false;
}

void ClientConfig::set_config(int argc, char const* const argv[], ClientRedemptionConfig & config) {
    ClientConfig::setDefaultConfig(config);
    if (!config.MAIN_DIR.empty()) {
        ClientConfig::setUserProfil(config);
        ClientConfig::setClientInfo(config);
        ClientConfig::setAccountData(config);
        ClientConfig::openWindowsData(config);
    }
    ClientConfig::parse_options(argc, argv, config);
}

void ClientConfig::setAccountData(ClientRedemptionConfig & config)  {
    config._accountNB = 0;

    unique_fd fd = unique_fd(config.USER_CONF_LOG.c_str(), O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);

    if (fd.is_open()) {
        int accountNB(0);
        std::string line;

        while(read_line(fd.fd(), line)) {
            auto pos(line.find(' '));
            std::string info = line.substr(pos + 1);

            if (line.compare(0, pos, "save_pwd") == 0) {
                config._save_password_account = (info == "true");
            } else
            if (line.compare(0, pos, "last_target") == 0) {
                config._last_target_index = std::stoi(info);
            } else
            if (line.compare(0, pos, "title") == 0) {
                AccountData new_account;
                config._accountData.push_back(new_account);
                config._accountData.back().title = info;
            } else
            if (line.compare(0, pos, "IP") == 0) {
                config._accountData.back().IP = info;
            } else
            if (line.compare(0, pos, "name") == 0) {
                config._accountData.back().name = info;
            } else if (line.compare(0, pos, "protocol") == 0) {
                config._accountData.back().protocol = std::stoi(info);
            } else
            if (line.compare(0, pos, "pwd") == 0) {
                config._accountData.back().pwd = info;
            } else
            if (line.compare(0, pos, "options_profil") == 0) {

                config._accountData.back().options_profil = std::stoi(info);
                config._accountData.back().index = accountNB;

                accountNB++;
            } else
            if (line.compare(0, pos, "port") == 0) {
                config._accountData.back().port = std::stoi(info);
            }
        }

        if (config._accountNB < int(config._accountData.size())) {
            config._accountNB = accountNB;
        }
    }
}


void ClientConfig::writeAccoundData(const std::string& ip, const std::string& name, const std::string& pwd, const int port, ClientRedemptionConfig & config)  {
    if (config.connected) {
        bool alreadySet = false;

        std::string title(ip + " - " + name);

        for (int i = 0; i < config._accountNB; i++) {
            if (config._accountData[i].IP == ip && config._accountData[i].name == name) {
                alreadySet = true;
                config._last_target_index = i;
                config._accountData[i].pwd  = pwd;
                config._accountData[i].port = port;
                config._accountData[i].options_profil  = config.current_user_profil;
            }
        }

        if (!alreadySet) {
            AccountData new_account;
            config._accountData.push_back(new_account);
            config._accountData[config._accountNB].title = title;
            config._accountData[config._accountNB].IP    = ip;
            config._accountData[config._accountNB].name  = name;
            config._accountData[config._accountNB].pwd   = pwd;
            config._accountData[config._accountNB].port  = port;
            config._accountData[config._accountNB].options_profil  = config.current_user_profil;
            config._accountData[config._accountNB].protocol = config.mod_state;
            config._accountNB++;

            config._last_target_index = config._accountNB;
        }

        unique_fd file = unique_fd(config.USER_CONF_LOG.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if(file.is_open()) {

            std::string to_write = str_concat(
                (config._save_password_account ? "save_pwd true\n" : "save_pwd false\n"),
                "last_target ",
                int_to_decimal_chars(config._last_target_index),
                "\n\n");

            for (int i = 0; i < config._accountNB; i++) {
                str_append(
                    to_write,
                    "title ", config._accountData[i].title, "\n"
                    "IP "   , config._accountData[i].IP   , "\n"
                    "name " , config._accountData[i].name , "\n"
                    "protocol ", int_to_decimal_chars(config._accountData[i].protocol), '\n');

                if (config._save_password_account) {
                    str_append(to_write, "pwd ", config._accountData[i].pwd, "\n");
                } else {
                    to_write += "pwd \n";
                }

                str_append(
                    to_write,
                    "port ", int_to_decimal_chars(config._accountData[i].port), "\n"
                    "options_profil ", int_to_decimal_chars(config._accountData[i].options_profil), "\n"
                    "\n");
            }

            ::write(file.fd(), to_write.c_str(), to_write.length());
        }
    }
}


void ClientConfig::deleteCurrentProtile(ClientRedemptionConfig & config)  {
    unique_fd file_to_read = unique_fd(config.USER_CONF_PATH.c_str(), O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if(file_to_read.is_open()) {

        std::string new_file_content = "current_user_profil_id 0\n";
        int ligne_to_jump = 0;

        std::string line;

        ClientConfig::read_line(file_to_read.fd(), line);

        while(ClientConfig::read_line(file_to_read.fd(), line)) {
            if (ligne_to_jump == 0) {
                std::string::size_type pos = line.find(' ');
                std::string info = line.substr(pos + 1u);

                if (line.compare(0, pos, "id") == 0 && std::stoi(info) == config.current_user_profil) {
                    ligne_to_jump = 18;
                } else {
                    str_append(new_file_content, line, '\n');
                }
            } else {
                ligne_to_jump--;
            }
        }

        file_to_read.close();

        unique_fd file_to_read = unique_fd(config.USER_CONF_PATH.c_str(), O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
            ::write(file_to_read.fd(), new_file_content.c_str(), new_file_content.length());
    }
}



void ClientConfig::writeClientInfo(ClientRedemptionConfig & config)  {

    unique_fd file = unique_fd(config.USER_CONF_PATH.c_str(), O_WRONLY| O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

    if(file.is_open()) {

        std::string to_write = str_concat(
            "current_user_profil_id ", int_to_decimal_chars(config.current_user_profil), '\n');

        bool not_reading_current_profil = true;
        std::string ligne;
        while(read_line(file.fd(), ligne)) {
            if (!ligne.empty()) {
                std::size_t pos = ligne.find(' ');

                if (ligne.compare(pos+1, std::string::npos, "id") == 0) {
                    to_write += '\n';
                    int read_id = std::stoi(ligne);
                    not_reading_current_profil = !(read_id == config.current_user_profil);
                }

                if (not_reading_current_profil) {
                    str_append(to_write, '\n', ligne);
                }
            }
        }

        str_append(
            to_write,
            "\nid ", int_to_decimal_chars(config.userProfils[config.current_user_profil].id), "\n"
            "name ", config.userProfils[config.current_user_profil].name, "\n"
            "keylayout ", int_to_decimal_chars(underlying_cast(config.info.keylayout)), "\n"
            "brush_cache_code ", int_to_decimal_chars(config.info.brush_cache_code), "\n"
            "bpp ", int_to_decimal_chars(underlying_cast(config.info.screen_info.bpp)), "\n"
            "width ", int_to_decimal_chars(config.info.screen_info.width), "\n"
            "height ", int_to_decimal_chars(config.info.screen_info.height), "\n"
            "rdp5_performanceflags ", int_to_decimal_chars(config.info.rdp5_performanceflags), "\n"
            "monitorCount ", int_to_decimal_chars(config.info.cs_monitor.monitorCount), "\n"
            "span ", int_to_decimal_chars(config.is_spanning), "\n"
            "record ", int_to_decimal_chars(config.is_recording),"\n"
            "tls ", int_to_decimal_chars(config.modRDPParamsData.enable_tls), "\n"
            "nla ", int_to_decimal_chars(config.modRDPParamsData.enable_nla), "\n"
            "tls-min-level ", int_to_decimal_chars(config.tls_client_params_data.tls_min_level), "\n"
            "tls-max-level ", int_to_decimal_chars(config.tls_client_params_data.tls_max_level), "\n"
            "tls-cipher-string ", config.tls_client_params_data.cipher_string, "\n"
            "show_common_cipher_list ", int_to_decimal_chars(config.tls_client_params_data.show_common_cipher_list), "\n"
            "sound ", int_to_decimal_chars(config.modRDPParamsData.enable_sound), "\n"
            "console_mode ", int_to_decimal_chars(config.info.console_session), "\n"
            "remotefx ", int_to_decimal_chars(config.enable_remotefx), "\n"
            "enable_shared_clipboard ", int_to_decimal_chars(config.enable_shared_clipboard), "\n"
            "enable_shared_virtual_disk ", int_to_decimal_chars(config.modRDPParamsData.enable_shared_virtual_disk), "\n"
            "enable_shared_remoteapp ", int_to_decimal_chars(config.modRDPParamsData.enable_shared_remoteapp), "\n"
            "share-dir ", config.SHARE_DIR, "\n"
            "remote-exe ", config.rDPRemoteAppConfig.full_cmd_line, "\n"
            "remote-dir ", config.rDPRemoteAppConfig.source_of_WorkingDir, "\n"
            "vnc-applekeyboard ", int_to_decimal_chars(config.modVNCParamsData.is_apple), "\n"
            "mod ", int_to_decimal_chars(config.mod_state) , "\n"
        );

        ::write(file.fd(), to_write.c_str(), to_write.length());
    }
}


void ClientConfig::setDefaultConfig(ClientRedemptionConfig & config)  {
    //config.current_user_profil = 0;
    config.info.build = 420;
    config.info.keylayout = KeyLayout::KbdId(0x040C);// 0x40C FR, 0x409 USA
    config.info.brush_cache_code = 0;
    config.info.screen_info.bpp = BitsPerPixel{24};
    config.info.screen_info.width  = 800;
    config.info.screen_info.height = 600;
    config.info.console_session = false;
    config.info.rdp5_performanceflags   = PERF_DISABLE_WALLPAPER;
    config.info.cs_monitor.monitorCount = 1;
    config.is_spanning = false;
    config.is_recording = false;
    config.modRDPParamsData.enable_tls = true;
    config.modRDPParamsData.enable_nla = true;
    config.tls_client_params_data.tls_min_level = 0;
    config.tls_client_params_data.tls_max_level = 0;
    // config.tls_client_params_data.cipher_string;
    config.tls_client_params_data.show_common_cipher_list = false;
    config.enable_shared_clipboard = true;
    config.modRDPParamsData.enable_shared_virtual_disk = true;
    config.SHARE_DIR = "/home";
    //config.info.encryptionLevel = 1;

    config.rDPRemoteAppConfig.source_of_ExeOrFile  = "C:\\Windows\\system32\\notepad.exe";
    config.rDPRemoteAppConfig.source_of_WorkingDir = "C:\\Users\\user1";

    config.rDPRemoteAppConfig.full_cmd_line = config.rDPRemoteAppConfig.source_of_ExeOrFile + " " + config.rDPRemoteAppConfig.source_of_Arguments;

    if (!config.MAIN_DIR.empty()) {
        for (auto* pstr : {
            &config.DATA_DIR,
            &config.REPLAY_DIR,
            &config.CB_TEMP_DIR,
            &config.DATA_CONF_DIR,
            &config.SOUND_TEMP_DIR
            // &config.WINDOWS_CONF
        }) {
            if (!pstr->empty()) {
                if (!file_exist(pstr->c_str())) {
                    LOG(LOG_INFO, "Create file \"%s\".", pstr->c_str());
                    mkdir(pstr->c_str(), 0775);
                }
            }
        }
    }

    // Set RDP CLIPRDR config
    config.rDPClipboardConfig.arbitrary_scale = 40;
    config.rDPClipboardConfig.server_use_long_format_names = true;
    config.rDPClipboardConfig.cCapabilitiesSets = 1;
    config.rDPClipboardConfig.generalFlags = RDPECLIP::CB_STREAM_FILECLIP_ENABLED | RDPECLIP::CB_FILECLIP_NO_FILE_PATHS;
    config.rDPClipboardConfig.add_format(ClientCLIPRDRConfig::CF_QT_CLIENT_FILEGROUPDESCRIPTORW, Cliprdr::formats::file_group_descriptor_w);
    config.rDPClipboardConfig.add_format(ClientCLIPRDRConfig::CF_QT_CLIENT_FILECONTENTS, Cliprdr::formats::file_contents);
    config.rDPClipboardConfig.add_format(RDPECLIP::CF_TEXT);
    config.rDPClipboardConfig.add_format(RDPECLIP::CF_METAFILEPICT);
    config.rDPClipboardConfig.path = config.CB_TEMP_DIR;

    // Set RDP RDPDR config
    config.rDPDiskConfig.add_drive(config.SHARE_DIR, rdpdr::RDPDR_DTYP_FILESYSTEM);
    config.rDPDiskConfig.enable_drive_type = true;
    config.rDPDiskConfig.enable_printer_type = true;

    // Set RDP SND config
    config.rDPSoundConfig.dwFlags = rdpsnd::TSSNDCAPS_ALIVE | rdpsnd::TSSNDCAPS_VOLUME;
    config.rDPSoundConfig.dwVolume = 0x7fff7fff;
    config.rDPSoundConfig.dwPitch = 0;
    config.rDPSoundConfig.wDGramPort = 0;
    config.rDPSoundConfig.wNumberOfFormats = 1;
    config.rDPSoundConfig.wVersion = 0x06;

    config.userProfils.emplace_back(0, "Default");

    std::fill(std::begin(config.info.order_caps.orderSupport), std::end(config.info.order_caps.orderSupport), 1);
    config.info.glyph_cache_caps.GlyphSupportLevel = GlyphCacheCaps::GLYPH_SUPPORT_FULL;
}

void ClientConfig::setUserProfil(ClientRedemptionConfig & config)  {
    unique_fd fd = unique_fd(config.USER_CONF_PATH.c_str(), O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd.is_open()) {
        std::string line;
        read_line(fd.fd(), line);
        auto pos(line.find(' '));
        if (line.compare(0, pos, "current_user_profil_id") == 0) {
            config.current_user_profil = std::stoi(line.substr(pos + 1));
        }
    }
}

void ClientConfig::setClientInfo(ClientRedemptionConfig & config)  {
    config.windowsData.config_file_path = config.WINDOWS_CONF;

    config.userProfils.clear();
    config.userProfils.emplace_back(0, "Default");

    unique_fd fd = unique_fd(config.USER_CONF_PATH.c_str(), O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);

    if (fd.is_open()) {

        int read_id(-1);
        std::string line;

        while(read_line(fd.fd(), line)) {

            auto pos(line.find(' '));
            std::string info = line.substr(pos + 1);
            std::string tag = line.substr(0, pos);
            if (tag == "id") {
                read_id = std::stoi(info);
            } else
            if (tag == "name") {
                if (read_id) {
                    config.userProfils.emplace_back(read_id, info);
                }
            } else
            if (config.current_user_profil == read_id) {

                if (tag == "keylayout") {
                    config.info.keylayout = KeyLayout::KbdId(std::stoi(info));
                } else
                if (tag == "console_session") {
                    config.info.console_session = std::stoi(info);
                } else
                if (tag == "remotefx") {
                    config.enable_remotefx = std::stoi(info);
                } else
                if (tag == "brush_cache_code") {
                    config.info.brush_cache_code = std::stoi(info);
                } else
                if (tag == "bpp") {
                    config.info.screen_info.bpp = checked_int(std::stoi(info));
                } else
                if (tag == "width") {
                    config.info.screen_info.width = std::stoi(info);
                } else
                if (tag == "height") {
                    config.info.screen_info.height = std::stoi(info);
                } else
                if (tag == "monitorCount") {
                    config.info.cs_monitor.monitorCount = std::stoi(info);
                } else
                if (tag == "span") {
                    config.is_spanning = bool(std::stoi(info));
                } else
                if (tag == "record") {
                    config.is_recording = bool(std::stoi(info));
                } else
                if (tag == "tls") {
                    config.modRDPParamsData.enable_tls = bool(std::stoi(info));
                } else
                if (tag == "tls-min-level") {
                    config.tls_client_params_data.tls_min_level = std::stoi(info);
                } else
                if (tag == "tls-max-level") {
                    config.tls_client_params_data.tls_max_level = std::stoi(info);
                } else
                if (tag == "tls-cipher-string") {
                    config.tls_client_params_data.cipher_string = info;
                } else
                if (tag == "show_common_cipher_list") {
                    config.tls_client_params_data.show_common_cipher_list = bool(std::stoi(info));
                } else
                if (tag == "nla") {
                    config.modRDPParamsData.enable_nla = bool(std::stoi(info));
                } else
                if (tag == "sound") {
                    config.modRDPParamsData.enable_sound = bool(std::stoi(info));
                } else
                if (tag == "console_mode") {
                    config.info.console_session = (std::stoi(info) > 0);
                } else
                if (tag == "enable_shared_clipboard") {
                    config.enable_shared_clipboard = bool(std::stoi(info));
                } else
                if (tag == "enable_shared_remoteapp") {
                    config.modRDPParamsData.enable_shared_remoteapp = bool(std::stoi(info));
                } else
                if (tag == "enable_shared_virtual_disk") {
                    config.modRDPParamsData.enable_shared_virtual_disk = bool(std::stoi(info));
                } else
                if (tag == "share-dir") {
                    config.SHARE_DIR = info;
                } else
                if (tag == "remote-exe") {
                    config.rDPRemoteAppConfig.full_cmd_line = info;
                    auto arfs_pos(info.find(' '));
                    if (arfs_pos == 0) {
                        config.rDPRemoteAppConfig.source_of_ExeOrFile = info;
                        config.rDPRemoteAppConfig.source_of_Arguments.clear();
                    }
                    else {
                        config.rDPRemoteAppConfig.source_of_ExeOrFile = info.substr(0, arfs_pos);
                        config.rDPRemoteAppConfig.source_of_Arguments = info.substr(arfs_pos + 1);
                    }
                } else
                if (tag == "remote-dir") {
                    config.rDPRemoteAppConfig.source_of_WorkingDir = info;
                } else
                if (tag == "rdp5_performanceflags") {
                    config.info.rdp5_performanceflags = std::stoi(info);
                } else
                if (tag == "vnc-applekeyboard ") {
                    config.modVNCParamsData.is_apple = bool(std::stoi(info));
                } else
                if (tag == "mod") {
                    config.mod_state = std::stoi(info);

                    read_id = -1;
                }
            }
        }
    }
}
