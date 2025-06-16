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
    Copyright (C) Wallix 2015
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#pragma once

#include "configs/config.hpp"
#include "utils/log.hpp"
#include "core/misc.hpp"
#include "core/RDP/windows_execute_shell_params.hpp"
#include "mod/rdp/channels/base_channel.hpp"
#include "mod/rdp/channels/rail_session_manager.hpp"
#include "mod/rdp/channels/sespro_channel.hpp"
#include "mod/rdp/channels/rail_window_id_manager.hpp"
#include "mod/rdp/rdp_api.hpp"
#include "mod/rdp/mod_rdp_variables.hpp"
#include "core/stream_throw_helpers.hpp"
#include "utils/ascii.hpp"


class FrontAPI;

struct RemoteProgramsVirtualChannelParams
{
    WindowsExecuteShellParams windows_execute_shell_params;
    WindowsExecuteShellParams windows_execute_shell_params_2;

    RemoteProgramsSessionManager* rail_session_manager;

    bool should_ignore_first_client_execute;

    bool use_session_probe_to_launch_remote_program;

    bool client_supports_handshakeex_pdu;
    bool client_supports_enhanced_remoteapp;
};


class RemoteProgramsVirtualChannel final : public BaseVirtualChannel,
    public rdp_api
{
private:
    uint16_t client_order_type = 0;
    uint16_t server_order_type = 0;

    WindowsExecuteShellParams windows_execute_shell_params;

    WindowsExecuteShellParams windows_execute_shell_params_2;

    RemoteProgramsSessionManager * param_rail_session_manager = nullptr;

    bool param_should_ignore_first_client_execute;

    bool param_use_session_probe_to_launch_remote_program;

    //bool param_client_supports_handshakeex_pdu;
    //bool param_client_supports_enhanced_remoteapp;

    bool first_client_execute_ignored = false;

    bool client_execute_pdu_sent = false;

    SessionProbeVirtualChannel * session_probe_channel = nullptr;

    SessionProbeLauncher* session_probe_stop_launch_sequence_notifier = nullptr;

    SessionLogApi& session_log;
    RDPVerbose verbose;

    bool exe_or_file_exec_ok = false;
    bool session_probe_launch_confirmed = false;

    bool exe_or_file_2_sent = false;

    ModRdpVariables vars;

    struct LaunchPendingApp
    {
        std::string original_exe_or_file;
        std::string exe_or_file;
        uint16_t    flags;
    };

    std::vector<LaunchPendingApp> launch_pending_apps;

    bool proxy_managed;

    uint16_t desktop_width;
    uint16_t desktop_height;

public:

    RemoteProgramsVirtualChannel(
        VirtualChannelDataSender* to_client_sender_,
        VirtualChannelDataSender* to_server_sender_,
        FrontAPI& /*front*/,
        bool proxy_managed,
        uint16_t desktop_width,
        uint16_t desktop_height,
        ModRdpVariables vars,
        const RemoteProgramsVirtualChannelParams& params,
        SessionLogApi& session_log,
        RDPVerbose verbose)
    : BaseVirtualChannel(to_client_sender_, to_server_sender_)
    , windows_execute_shell_params(params.windows_execute_shell_params)
    , windows_execute_shell_params_2(params.windows_execute_shell_params_2)
    , param_rail_session_manager(params.rail_session_manager)
    , param_should_ignore_first_client_execute(params.should_ignore_first_client_execute)
    , param_use_session_probe_to_launch_remote_program(params.use_session_probe_to_launch_remote_program)
    //, param_client_supports_handshakeex_pdu(params.client_supports_handshakeex_pdu)
    //, param_client_supports_enhanced_remoteapp(params.client_supports_enhanced_remoteapp)
    , session_log(session_log)
    , verbose(verbose)
    , vars(vars)
    , proxy_managed(proxy_managed)
    , desktop_width(desktop_width)
    , desktop_height(desktop_height)
    {}

private:
    template<class PDU>
    bool process_client_windowing_pdu(uint32_t flags, InStream& chunk) {
        PDU pdu;

        pdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            pdu.log(LOG_INFO);
        }

        bool is_client_only_window = true;
        try
        {
            is_client_only_window = this->param_rail_session_manager->is_client_only_window(pdu.WindowId());
        }
        catch (Error const& e)
        {
            if (ERR_RAIL_NO_SUCH_WINDOW_EXIST != e.id) {
                throw;
            }
        }
        if (is_client_only_window) {
            return false;
        }

        if (pdu.map_window_id(*this->param_rail_session_manager)) {
            StaticOutStream<65536> out_s;
            RAILPDUHeader          header;

            header.emit_begin(out_s, PDU::orderType());
            pdu.emit(out_s);
            header.emit_end();

            this->send_message_to_server(out_s.get_offset(), flags, out_s.get_produced_bytes());

            return false;
        }

        return true;
    }

    template<class PDU>
    bool process_server_windowing_pdu(uint32_t flags, InStream& chunk) {
        PDU pdu;

        pdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            pdu.log(LOG_INFO);
        }

        if (this->param_rail_session_manager->is_server_only_window(pdu.WindowId())) {
            return false;
        }

        if (pdu.map_window_id(*this->param_rail_session_manager)) {
            StaticOutStream<65536> out_s;
            RAILPDUHeader          header;

            header.emit_begin(out_s, PDU::orderType());
            pdu.emit(out_s);
            header.emit_end();

            this->send_message_to_client(out_s.get_offset(), flags, out_s.get_produced_bytes());

            return false;
        }

        return true;
    }

    // Check if a PDU chunk is a "unit" on the channel, which means
    // - it has both FLAG_FIRST and FLAG_LAST
    // - it contains at least two bytes (to read the chunk length)
    // - its total length is the same as the chunk length
    static void check_is_unit_throw(uint32_t total_length, uint32_t flags, InStream& chunk, const char * message)
    {
        if ((flags & (CHANNELS::CHANNEL_FLAG_FIRST|CHANNELS::CHANNEL_FLAG_LAST))
                  != (CHANNELS::CHANNEL_FLAG_FIRST|CHANNELS::CHANNEL_FLAG_LAST)){
            LOG(LOG_ERR, "RemoteProgramsVirtualChannel::%s unexpected fragmentation flags=%.4x", message, flags);
            throw Error(ERR_RDP_DATA_CHANNEL_FRAGMENTATION);
        }

        // orderLength(2)
        if (!chunk.in_check_rem(2)) {
            LOG(LOG_ERR,
                "Truncated RemoteProgramsVirtualChannel::%s::orderLength: expected=2 remains=%zu",
                message, chunk.in_remain());
            throw Error(ERR_RDP_DATA_TRUNCATED);
        }

        auto order_length = chunk.in_uint16_le(); // orderLength(2)
        if (total_length != order_length){
            LOG(LOG_ERR, "RemoteProgramsVirtualChannel::%s unexpected fragmentation chunk=%u total=%u", message, order_length, total_length);
            throw Error(ERR_RDP_DATA_CHANNEL_FRAGMENTATION);
        }
    }


    bool process_client_activate_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_activate_pdu");
        return this->process_client_windowing_pdu<ClientActivatePDU>(flags, chunk);
    }

    bool process_client_compartment_status_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_compartment_statusinformation_pdu");

        CompartmentStatusInformationPDU csipdu;
        csipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            csipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_client_execute_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_execute_pdu");

        ClientExecutePDU cepdu;
        cepdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            cepdu.log(LOG_INFO);
        }

        if (this->param_should_ignore_first_client_execute
        && !this->first_client_execute_ignored) {
            this->first_client_execute_ignored = true;

            LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                "RemoteProgramsVirtualChannel::process_client_execute_pdu: "
                    "First Client Execute PDU ignored.");

            return false;
        }

        chars_view exe_of_file = cepdu.get_windows_execute_shell_params().exe_or_file;
        auto exe_of_file_upper = ascii_to_limited_upper<256>(exe_of_file);
        static_assert(sizeof(DUMMY_REMOTEAPP ":") <= 16);

        if (exe_of_file_upper.starts_with(DUMMY_REMOTEAPP ":"_ascii_upper))
        {
            auto remoteapplicationprogram = exe_of_file.drop_front(sizeof(DUMMY_REMOTEAPP ":") - 1)
              .as<std::string_view>();

            LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                "RemoteProgramsVirtualChannel::process_client_execute_pdu: "
                    "remoteapplicationprogram=\"%.*s\"",
                int(remoteapplicationprogram.size()), remoteapplicationprogram.data());

            this->vars.set_acl<cfg::context::auth_notify>("rail_exec");
            this->vars.set_acl<cfg::context::auth_notify_rail_exec_flags>(cepdu.get_windows_execute_shell_params().flags);
            this->vars.set_acl<cfg::context::auth_notify_rail_exec_exe_or_file>(remoteapplicationprogram);
        }
        else if (exe_of_file_upper != DUMMY_REMOTEAPP ""_ascii_upper) {
            return true;
        }

        return false;
    }

    bool process_client_get_application_id_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_get_application_id_pdu");
        return this->process_client_windowing_pdu<ClientGetApplicationIDPDU>(flags, chunk);
    }

    bool process_client_handshake_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_handshake_pdu");

        HandshakePDU hspdu;
        hspdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            hspdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_client_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_information_pdu");

        ClientInformationPDU cipdu;
        cipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            cipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_client_language_bar_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_language_bar_information_pdu");

        LanguageBarInformationPDU lbipdu;
        lbipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            lbipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_client_language_profile_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_language_profile_information_pdu");

        LanguageProfileInformationPDU lpipdu;
        lpipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            lpipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_client_notify_event_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_notify_event_pdu");
        return this->process_client_windowing_pdu<ClientNotifyEventPDU>(flags, chunk);
    }

    bool process_client_system_command_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_system_command_pdu");
        return this->process_client_windowing_pdu<ClientSystemCommandPDU>(flags, chunk);
    }

    bool process_client_system_parameters_update_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_system_parameters_update_pdu");

        ClientSystemParametersUpdatePDU cspupdu;
        cspupdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            cspupdu.log(LOG_INFO);
        }

        if (!this->client_execute_pdu_sent) {
            if (!this->windows_execute_shell_params.exe_or_file.empty()) {
                StaticOutStream<16384> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_EXEC);


                //TODO: define a constructor providing a WindowsExecuteShellParams structure
                ClientExecutePDU cepdu;

                cepdu.Flags(this->windows_execute_shell_params.flags);
                cepdu.ExeOrFile(this->windows_execute_shell_params.exe_or_file);
                cepdu.WorkingDir(this->windows_execute_shell_params.working_dir);
                cepdu.Arguments(this->windows_execute_shell_params.arguments);

                cepdu.emit(out_s);

                header.emit_end();

                const size_t   length = out_s.get_offset();
                const uint32_t flags  =   CHANNELS::CHANNEL_FLAG_FIRST
                                        | CHANNELS::CHANNEL_FLAG_LAST;

                {
                    const bool send              = true;
                    const bool from_or_to_client = false;
                    ::msgdump_c(send, from_or_to_client, length, flags, out_s.get_produced_bytes());
                }
                if (bool(this->verbose & RDPVerbose::rail)) {
                    LOG(LOG_INFO,
                        "RemoteProgramsVirtualChannel::process_client_system_parameters_update_pdu: "
                            "Send to server - Client Execute PDU");
                    cepdu.log(LOG_INFO);
                }

                this->send_message_to_server(length, flags, out_s.get_produced_bytes());
            }

            this->client_execute_pdu_sent = true;
        }

        return true;
    }

    bool process_client_system_menu_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_system_menu_pdu");
        return this->process_client_windowing_pdu<ClientSystemMenuPDU>(flags, chunk);
    }

    bool process_client_window_cloak_state_change_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_window_cloak_state_change_pdu");
        WindowCloakStateChangePDU wcscpdu;

        wcscpdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            wcscpdu.log(LOG_INFO);
        }

        return this->process_client_windowing_pdu<WindowCloakStateChangePDU>(flags, chunk);
    }

    bool process_client_window_move_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_client_window_move_pdu");
        return this->process_client_windowing_pdu<ClientWindowMovePDU>(flags, chunk);
    }

public:
    void process_client_message(uint32_t total_length,
        uint32_t flags, bytes_view chunk_data) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
            "RemoteProgramsVirtualChannel::process_client_message: "
                "total_length=%u flags=0x%08X chunk_data_length=%zu",
            total_length, flags, chunk_data.size());

        if (bool(this->verbose & RDPVerbose::rail_dump)) {
            const bool send              = false;
            const bool from_or_to_client = true;
            ::msgdump_c(send, from_or_to_client, total_length, flags, chunk_data);
        }

        InStream  chunk(chunk_data);

        // TODO: see that, order type lifetime seems much too long and not controlled
        if (flags & CHANNELS::CHANNEL_FLAG_FIRST) {
            // orderType(2)
            ::check_throw(chunk, 2, "RemoteProgramsVirtualChannel::process_client_message", ERR_RDP_DATA_TRUNCATED);
            this->client_order_type = chunk.in_uint16_le();
        }

        bool send_message_to_server = true;

        switch (this->client_order_type)
        {
            case TS_RAIL_ORDER_ACTIVATE:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Activate PDU");

                send_message_to_server =
                    this->process_client_activate_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_CLIENTSTATUS:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Information PDU");

                send_message_to_server =
                    this->process_client_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_COMPARTMENTINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Compartment Status Information PDU");

                send_message_to_server =
                    this->process_client_compartment_status_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_CLOAK:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Window Cloak State Change PDU");

                send_message_to_server =
                    this->process_client_window_cloak_state_change_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_EXEC:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Execute PDU");

                send_message_to_server =
                    this->process_client_execute_pdu(
                        total_length, flags, chunk);

                if (send_message_to_server) this->client_execute_pdu_sent = true;
            break;

            case TS_RAIL_ORDER_GET_APPID_REQ:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Get Application ID PDU");

                send_message_to_server =
                    this->process_client_get_application_id_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_HANDSHAKE:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Handshake PDU");

                send_message_to_server =
                    this->process_client_handshake_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_LANGBARINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Language Bar Information PDU");

                send_message_to_server =
                    this->process_client_language_bar_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_LANGUAGEIMEINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Language Profile Information PDU");

                send_message_to_server =
                    this->process_client_language_profile_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_NOTIFY_EVENT:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Notify Event PDU");

                send_message_to_server =
                    this->process_client_notify_event_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_SYSCOMMAND:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client System Command PDU");

                send_message_to_server =
                    this->process_client_system_command_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_SYSPARAM:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client System Parameters Update PDU");

                send_message_to_server =
                    this->process_client_system_parameters_update_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_SYSMENU:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client System Menu PDU");

                send_message_to_server =
                    this->process_client_system_menu_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_WINDOWMOVE:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Client Window Move PDU");

                send_message_to_server =
                    this->process_client_window_move_pdu(
                        total_length, flags, chunk);
            break;

            default:
                LOG(LOG_WARNING,
                    "RemoteProgramsVirtualChannel::process_client_message: "
                        "Delivering unprocessed messages %s(%u) to server.",
                    get_RAIL_orderType_name(this->client_order_type),
                    static_cast<unsigned>(this->client_order_type));

//                assert(false);
            break;
        }   // switch (this->client_order_type)

        if (send_message_to_server) {
            this->send_message_to_server(total_length, flags, chunk_data);
        }
    }   // process_client_message

    bool process_server_compartment_status_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_compartment_status_information_pdu");
        CompartmentStatusInformationPDU csipdu;

        csipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            csipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_server_execute_result_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        // TODO: see use of is_auth_application, looks like it is used for
        // executionflow control. We should avoid to do that
        bool is_auth_application = false;

        this->check_is_unit_throw(total_length, flags, chunk, "process_server_execute_result_pdu");

        ServerExecuteResultPDU serpdu;
        serpdu.receive(chunk);
        if (bool(this->verbose & RDPVerbose::rail)) {
            serpdu.log(LOG_INFO);
        }

        auto iter = std::find_if(
                this->launch_pending_apps.begin(),
                this->launch_pending_apps.end(),
                [&serpdu](LaunchPendingApp const& app) -> bool {
                    return app.original_exe_or_file == serpdu.ExeOrFile()
                        && app.flags == serpdu.Flags();
                }
            );
        if (this->launch_pending_apps.end() != iter) {
            LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                "RemoteProgramsVirtualChannel::process_server_execute_result_pdu: "
                    "Bastion Application found. OriginalExeOrFile=\"%s\" "
                    "ExeOrFile=\"%s\"",
                iter->original_exe_or_file, serpdu.ExeOrFile());

            serpdu.ExeOrFile(iter->original_exe_or_file);

            this->launch_pending_apps.erase(iter);

            is_auth_application = true;
        }

        if (serpdu.ExecResult() != RAIL_EXEC_S_OK) {
            uint16_t ExecResult_ = serpdu.ExecResult();
            LOG(LOG_WARNING,
                "RemoteProgramsVirtualChannel::process_server_execute_result_pdu: "
                    "Flags=0x%X ExecResult=%s(%d) RawResult=%u",
                serpdu.Flags(),
                get_RAIL_ExecResult_name(ExecResult_), ExecResult_,
                serpdu.RawResult());
        }
        else {
            if (!this->session_probe_channel
             || this->windows_execute_shell_params.exe_or_file != serpdu.ExeOrFile()
            ) {
                this->session_log.log6(LogId::CLIENT_EXECUTE_REMOTEAPP, {
                    KVLog("exe_or_file"_av, serpdu.ExeOrFile()),
                });
            }
        }

        if (this->windows_execute_shell_params.exe_or_file == serpdu.ExeOrFile()) {
            assert(!is_auth_application);

            if (this->session_probe_channel) {
/*
                if (this->session_probe_stop_launch_sequence_notifier) {
                    this->session_probe_stop_launch_sequence_notifier->stop(serpdu.ExecResult() == RAIL_EXEC_S_OK);
                    this->session_probe_stop_launch_sequence_notifier = nullptr;
                }
*/
                if (serpdu.ExecResult() != RAIL_EXEC_S_OK) {
                    throw Error(ERR_SESSION_PROBE_RP_LAUNCH_REFER_TO_SYSLOG);
                }

                if (!this->exe_or_file_2_sent &&
                    !this->windows_execute_shell_params_2.exe_or_file.empty()) {
                    this->exe_or_file_exec_ok = true;

                    this->try_launch_application();
                }
            }
            else {
                if (serpdu.ExecResult() != RAIL_EXEC_S_OK) {
                    throw Error(ERR_RAIL_CLIENT_EXECUTE);
                }
            }

            return (!this->session_probe_channel);
        }

        if (this->windows_execute_shell_params_2.exe_or_file == serpdu.ExeOrFile()) {
            assert(!is_auth_application);

            if (this->session_probe_channel) {
                this->session_probe_channel->start_end_session_check();
            }

            if (serpdu.ExecResult() != RAIL_EXEC_S_OK) {
                throw Error((serpdu.ExecResult() == RAIL_EXEC_E_NOT_IN_ALLOWLIST)
                    ? ERR_RAIL_UNAUTHORIZED_PROGRAM
                    : ERR_RAIL_STARTING_PROGRAM);
            }

            return true;
        }

        if (is_auth_application) {
            StaticOutStream<1024> out_s;
            RAILPDUHeader header;
            header.emit_begin(out_s, TS_RAIL_ORDER_EXEC_RESULT);

            serpdu.emit(out_s);

            header.emit_end();

            const size_t length = out_s.get_offset();
            const uint32_t flags_ =   CHANNELS::CHANNEL_FLAG_FIRST
                                    | CHANNELS::CHANNEL_FLAG_LAST;
            {
                const bool send              = true;
                const bool from_or_to_client = true;
                ::msgdump_c(send, from_or_to_client, length, flags_, out_s.get_produced_bytes());
            }
            if (bool(this->verbose & RDPVerbose::rail)) {
                LOG(LOG_INFO,
                    "RemoteProgramsVirtualChannel::auth_rail_exec_cancel: "
                        "Send to client - Server Execute Result PDU");
                serpdu.log(LOG_INFO);
            }

            this->send_message_to_client(length, flags_, out_s.get_produced_bytes());

            return false;
        }

        return true;
    }

    bool process_server_get_application_id_response_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_get_application_id_response_pdu");
        return this->process_server_windowing_pdu<ServerGetApplicationIDResponsePDU>(flags, chunk);
    }

    bool process_server_handshake_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_handshake_pdu");
        HandshakePDU hspdu;
        hspdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            hspdu.log(LOG_INFO);
        }

        // if (this->param_client_supports_enhanced_remoteapp &&
        //     this->param_client_supports_handshakeex_pdu) {

        //     HandshakeExPDU hsexpdu;

        //     hsexpdu.buildNumber(hspdu.buildNumber());
        //     hsexpdu.railHandshakeFlags(TS_RAIL_ORDER_HANDSHAKEEX_FLAGS_HIDEF);

        //     StaticOutStream<1024> out_s;
        //     RAILPDUHeader header;
        //     header.emit_begin(out_s, TS_RAIL_ORDER_HANDSHAKE_EX);

        //     hsexpdu.emit(out_s);

        //     header.emit_end();

        //     const size_t   length = out_s.get_offset();
        //     const uint32_t flags_ =   CHANNELS::CHANNEL_FLAG_FIRST
        //                             | CHANNELS::CHANNEL_FLAG_LAST;
        //     {
        //         const bool send              = true;
        //         const bool from_or_to_client = true;
        //         ::msgdump_c(send, from_or_to_client, length, flags_,
        //             out_s.get_data(), length);
        //     }
        //     if (bool(this->verbose & RDPVerbose::rail)) {
        //         LOG(LOG_INFO,
        //             "RemoteProgramsVirtualChannel::process_server_handshake_pdu: "
        //                 "Send to client - HandshakeEx PDU");
        //         hsexpdu.log(LOG_INFO);
        //     }

        //     this->send_message_to_client(length, flags_, out_s.get_data(),
        //         length);

        //     return false;
        // }

        return true;
    }

    bool process_server_handshake_ex_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_handshake_ex_pdu");
        HandshakeExPDU hsexpdu;
        hsexpdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            hsexpdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_server_language_bar_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_language_bar_information_pdu");
        LanguageBarInformationPDU lbipdu;
        lbipdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            lbipdu.log(LOG_INFO);
        }

        return true;
    }

    bool process_server_min_max_info_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_min_max_info_pdu");
        return this->process_server_windowing_pdu<ServerMinMaxInfoPDU>(flags, chunk);
    }

    bool process_server_move_size_start_or_end_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_move_size_start_or_end_pdu");
        return this->process_server_windowing_pdu<ServerMoveSizeStartOrEndPDU>(flags, chunk);
    }

    bool process_server_system_parameters_update_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_system_parameters_update_pdu");
        ServerSystemParametersUpdatePDU sspupdu;
        sspupdu.receive(chunk);

        if (bool(this->verbose & RDPVerbose::rail)) {
            sspupdu.log(LOG_INFO);
        }

        if (this->proxy_managed && !this->client_execute_pdu_sent && (SPI_SETSCREENSAVEACTIVE == sspupdu.SystemParam()))
        {
            auto send_to_server = [this](OutStream& out_s, auto& pdu, char const* stype){
                const size_t length = out_s.get_offset();
                const uint32_t flags_ = CHANNELS::CHANNEL_FLAG_FIRST
                                      | CHANNELS::CHANNEL_FLAG_LAST;
                {
                    const bool send              = true;
                    const bool from_or_to_client = true;
                    ::msgdump_c(send, from_or_to_client, length, flags_, out_s.get_produced_bytes());
                }
                if (bool(this->verbose & RDPVerbose::rail)) {
                    LOG(LOG_INFO,
                        "RemoteProgramsVirtualChannel::process_server_handshake_pdu: "
                            "Send to server - %s PDU", stype);
                    pdu.log(LOG_INFO);
                }

                this->send_message_to_server(length, flags_, out_s.get_produced_bytes());
            };

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_CLIENTSTATUS);

                ClientInformationPDU cipdu;
                cipdu.set_flags(0x1E7);
                cipdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, cipdu, "Client Information");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_HANDSHAKE);

                HandshakePDU hspdu;
                hspdu.buildNumber(17763);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Handshake");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_LANGBARINFO);

                LanguageBarInformationPDU hspdu;
                hspdu.LanguageBarStatus(0x498);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Language Bar Information");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                HighContrastSystemInformationStructure hcsis;
                hcsis.Flags(0x7E);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETHIGHCONTRAST);
                hspdu.body_hcsis(std::move(hcsis));
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                if (!this->windows_execute_shell_params.exe_or_file.empty()) {
                    StaticOutStream<16384> out_s;
                    RAILPDUHeader header;
                    header.emit_begin(out_s, TS_RAIL_ORDER_EXEC);

                    ClientExecutePDU cepdu;

                    cepdu.Flags(this->windows_execute_shell_params.flags);
                    cepdu.ExeOrFile(this->windows_execute_shell_params.exe_or_file);
                    cepdu.WorkingDir(this->windows_execute_shell_params.working_dir);
                    cepdu.Arguments(this->windows_execute_shell_params.arguments);

                    cepdu.emit(out_s);

                    header.emit_end();

                    send_to_server(out_s, cepdu, "Client Execute");
                }

                this->client_execute_pdu_sent = true;
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                RDP::RAIL::Rectangle rectangle(0, this->desktop_height - 40 /* Taskbar height */, this->desktop_width, this->desktop_height);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(RAIL_SPI_TASKBARPOS);
                hspdu.body_r(rectangle);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETMOUSEBUTTONSWAP);
                hspdu.body_b(0);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETKEYBOARDPREF);
                hspdu.body_b(0);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETDRAGFULLWINDOWS);
                hspdu.body_b(0);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETKEYBOARDCUES);
                hspdu.body_b(0);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            {
                StaticOutStream<1024> out_s;
                RAILPDUHeader header;
                header.emit_begin(out_s, TS_RAIL_ORDER_SYSPARAM);

                RDP::RAIL::Rectangle rectangle(0, 0, this->desktop_width - 2 /* ? */, this->desktop_height - 40 /* Taskbar height */);

                ClientSystemParametersUpdatePDU hspdu;
                hspdu.SystemParam(SPI_SETWORKAREA);
                hspdu.body_r(rectangle);
                hspdu.emit(out_s);

                header.emit_end();

                send_to_server(out_s, hspdu, "Client System Parameters Update");
            }

            return false;
        }

        return true;
    }

    bool process_server_z_order_sync_information_pdu(uint32_t total_length,
        uint32_t flags, InStream& chunk)
    {
        this->check_is_unit_throw(total_length, flags, chunk, "process_server_z_order_sync_information_pdu");
        return this->process_server_windowing_pdu<ServerZOrderSyncInformationPDU>(flags, chunk);
    }

    void process_server_message(uint32_t total_length,
        uint32_t flags, bytes_view chunk_data) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
            "RemoteProgramsVirtualChannel::process_server_message: "
                "total_length=%u flags=0x%08X chunk_data_length=%zu",
            total_length, flags, chunk_data.size());

        if (bool(this->verbose & RDPVerbose::rail_dump)) {
            const bool send              = false;
            const bool from_or_to_client = false;
            ::msgdump_c(send, from_or_to_client, total_length, flags, chunk_data);
        }

        if (flags && !(flags &~ (CHANNELS::CHANNEL_FLAG_SUSPEND | CHANNELS::CHANNEL_FLAG_RESUME))) {
            this->send_message_to_client(total_length, flags, chunk_data);
            return;
        }

        InStream chunk(chunk_data);

        if (flags & CHANNELS::CHANNEL_FLAG_FIRST) {
            // orderType(2)
            ::check_throw(chunk, 2, "RemoteProgramsVirtualChannel::process_server_message::orderLength", ERR_RDP_DATA_TRUNCATED);
            this->server_order_type = chunk.in_uint16_le(); // orderType(2)
        }

        bool send_message_to_client = true;

        switch (this->server_order_type)
        {
            case TS_RAIL_ORDER_COMPARTMENTINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Compartment Status Information PDU");

                send_message_to_client =
                    this->process_server_compartment_status_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_EXEC_RESULT:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Execute Result PDU");

                send_message_to_client =
                    this->process_server_execute_result_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_GET_APPID_RESP:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Get Application ID Response PDU");

                send_message_to_client =
                    this->process_server_get_application_id_response_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_HANDSHAKE:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Handshake PDU");

                send_message_to_client =
                    this->process_server_handshake_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_HANDSHAKE_EX:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server HandshakeEx PDU");

                send_message_to_client =
                    this->process_server_handshake_ex_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_LANGBARINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Language Bar Information PDU");

                send_message_to_client =
                    this->process_server_language_bar_information_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_LOCALMOVESIZE:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Move/Size Start/End PDU");

                send_message_to_client =
                    this->process_server_move_size_start_or_end_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_MINMAXINFO:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Min Max Info PDU");

                send_message_to_client =
                    this->process_server_min_max_info_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_SYSPARAM:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server System Parameters Update PDU");

                send_message_to_client =
                    this->process_server_system_parameters_update_pdu(
                        total_length, flags, chunk);
            break;

            case TS_RAIL_ORDER_ZORDER_SYNC:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Server Z-Order Sync Information PDU");

                send_message_to_client =
                    this->process_server_z_order_sync_information_pdu(
                        total_length, flags, chunk);
            break;

            default:
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::process_server_message: "
                        "Delivering unprocessed messages %s(%u) to client.",
                    get_RAIL_orderType_name(this->server_order_type),
                    static_cast<unsigned>(this->server_order_type));
                assert(false);
            break;
        }   // switch (this->server_order_type)

        if (send_message_to_client) {
            this->send_message_to_client(total_length, flags, chunk_data);
        }   // switch (this->server_order_type)
    }   // process_server_message

    void set_session_probe_virtual_channel(SessionProbeVirtualChannel * session_probe_channel) {
        this->session_probe_channel = session_probe_channel;
    }

    void set_session_probe_launcher(SessionProbeLauncher* launcher) {
        this->session_probe_stop_launch_sequence_notifier = launcher;
    }

    void auth_rail_exec(uint16_t flags, chars_view original_exe_or_file,
            chars_view exe_or_file, chars_view working_dir,
            chars_view arguments, chars_view account, chars_view password) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
            "RemoteProgramsVirtualChannel::auth_rail_exec: "
                "original_exe_or_file=\"%.*s\" "
                "exe_or_file=\"%.*s\" "
                "working_dir=\"%.*s\" "
                "arguments=\"%.*s\" "
                "account=\"%.*s\" "
                "flags=%u",
            int(original_exe_or_file.size()), original_exe_or_file.data(),
            int(exe_or_file.size()), exe_or_file.data(),
            int(working_dir.size()), working_dir.data(),
            int(arguments.size()), arguments.data(),
            int(account.size()), account.data(),
            flags);

        launch_pending_apps.emplace_back(LaunchPendingApp{
            original_exe_or_file.as<std::string>(),
            exe_or_file.as<std::string>(),
            flags
        });

        auto arguments_ = arguments.as<std::string>();

        utils::str_replace_inplace(arguments_, "${APPID}"_av, original_exe_or_file);

        if (!account.empty()) {
            utils::str_replace_inplace(arguments_, "${USER}"_av, account);
        }
        if (!password.empty()) {
            utils::str_replace_inplace(arguments_, "${PASSWORD}"_av, str_concat('\x03', password, '\x03'));
        }

        if (this->param_use_session_probe_to_launch_remote_program &&
            this->session_probe_channel) {
                this->session_probe_channel->rail_exec(
                        exe_or_file,
                        arguments_,
                        working_dir,
                        false,  // Show maximized
                        flags
                    );
                LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
                    "RemoteProgramsVirtualChannel::auth_rail_exec: "
                        "Send to Session Probe - Execure");
        }
        else {
            StaticOutStream<1024> out_s;
            RAILPDUHeader header;
            header.emit_begin(out_s, TS_RAIL_ORDER_EXEC);

            ClientExecutePDU cepdu;

            cepdu.Flags(flags);
            cepdu.ExeOrFile(exe_or_file.as<std::string_view>());
            cepdu.WorkingDir(working_dir.as<std::string_view>());
            cepdu.Arguments(arguments_);

            cepdu.emit(out_s);

            header.emit_end();

            const size_t   length = out_s.get_offset();
            const uint32_t flags_ =   CHANNELS::CHANNEL_FLAG_FIRST
                                    | CHANNELS::CHANNEL_FLAG_LAST;

            {
                const bool send              = true;
                const bool from_or_to_client = false;
                ::msgdump_c(send, from_or_to_client, length, flags_, out_s.get_produced_bytes());
            }
            if (bool(this->verbose & RDPVerbose::rail)) {
                LOG(LOG_INFO,
                    "RemoteProgramsVirtualChannel::auth_rail_exec: "
                        "Send to server - Client Execute PDU");
                cepdu.log(LOG_INFO);
            }

            this->send_message_to_server(length, flags_, out_s.get_produced_bytes());
        }
    }

    void auth_rail_exec_cancel(uint16_t flags, chars_view original_exe_or_file, uint16_t exec_result) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
            "RemoteProgramsVirtualChannel::auth_rail_exec_cancel: "
                "exec_result=%u "
                "original_exe_or_file=\"%.*s\" "
                "flags=%u",
            exec_result, int(original_exe_or_file.size()), original_exe_or_file.data(), flags);

        StaticOutStream<1024> out_s;
        RAILPDUHeader header;
        header.emit_begin(out_s, TS_RAIL_ORDER_EXEC_RESULT);

        ServerExecuteResultPDU serpdu;

        serpdu.Flags(flags);
        serpdu.ExecResult(exec_result);
        serpdu.RawResult(0xFFFFFFFF);
        serpdu.ExeOrFile(original_exe_or_file.as<std::string_view>());

        serpdu.emit(out_s);

        header.emit_end();

        const size_t   length = out_s.get_offset();
        const uint32_t flags_ =   CHANNELS::CHANNEL_FLAG_FIRST
                                | CHANNELS::CHANNEL_FLAG_LAST;
        {
            const bool send              = true;
            const bool from_or_to_client = true;
            ::msgdump_c(send, from_or_to_client, length, flags_, out_s.get_produced_bytes());
        }
        if (bool(this->verbose & RDPVerbose::rail)) {
            LOG(LOG_INFO,
                "RemoteProgramsVirtualChannel::auth_rail_exec_cancel: "
                    "Send to client - Server Execute Result PDU");
            serpdu.log(LOG_INFO);
        }

        this->send_message_to_client(length, flags_, out_s.get_produced_bytes());
    }

    void sespro_rail_exec_result(uint16_t flags, chars_view exe_or_file,
        uint16_t exec_result, uint32_t raw_result) override {
        LOG_IF(bool(this->verbose & RDPVerbose::rail), LOG_INFO,
            "RemoteProgramsVirtualChannel::sespro_rail_exec_result: "
                "exec_result=%u "
                "raw_result=%u "
                "exe_or_file=\"%.*s\" "
                "flags=%u",
            exec_result, raw_result, int(exe_or_file.size()), exe_or_file.data(), flags);

        StaticOutStream<1024> out_s;
        RAILPDUHeader header;
        header.emit_begin(out_s, TS_RAIL_ORDER_EXEC_RESULT);

        ServerExecuteResultPDU serpdu;

        serpdu.Flags(flags);
        serpdu.ExecResult(exec_result);
        serpdu.RawResult(raw_result);
        serpdu.ExeOrFile(exe_or_file.as<std::string>());

        serpdu.emit(out_s);

        header.emit_end();

        const size_t   length = out_s.get_offset();
        const uint32_t flags_ =   CHANNELS::CHANNEL_FLAG_FIRST
                                | CHANNELS::CHANNEL_FLAG_LAST;
        {
            const bool send              = true;
            const bool from_or_to_client = true;
            ::msgdump_c(send, from_or_to_client, length, flags_, out_s.get_produced_bytes());
        }
        if (bool(this->verbose & RDPVerbose::rail)) {
            LOG(LOG_INFO,
                "RemoteProgramsVirtualChannel::sespro_rail_exec_result: "
                    "Send to client - Server Execute Result PDU");
            serpdu.log(LOG_INFO);
        }

        this->send_message_to_client(length, flags_, out_s.get_produced_bytes());
    }

    void confirm_session_probe_launch() {
        if (!this->exe_or_file_2_sent &&
            !this->windows_execute_shell_params_2.exe_or_file.empty()) {
            this->session_probe_launch_confirmed = true;

            this->try_launch_application();
        }
    }

private:
    void try_launch_application() {
        if (!this->exe_or_file_exec_ok || !this->session_probe_launch_confirmed) {
            return;
        }

        assert(!this->exe_or_file_2_sent &&
            !this->windows_execute_shell_params_2.exe_or_file.empty());

        this->exe_or_file_2_sent = true;

        StaticOutStream<16384> out_s;
        RAILPDUHeader header;
        header.emit_begin(out_s, TS_RAIL_ORDER_EXEC);

        ClientExecutePDU cepdu;

        cepdu.Flags(this->windows_execute_shell_params_2.flags);
        cepdu.ExeOrFile(this->windows_execute_shell_params_2.exe_or_file);
        cepdu.WorkingDir(this->windows_execute_shell_params_2.working_dir);
        cepdu.Arguments(this->windows_execute_shell_params_2.arguments);

        cepdu.emit(out_s);

        header.emit_end();

        const size_t   length = out_s.get_offset();
        const uint32_t flags  =   CHANNELS::CHANNEL_FLAG_FIRST
                                | CHANNELS::CHANNEL_FLAG_LAST;

        {
            const bool send              = true;
            const bool from_or_to_client = false;
            ::msgdump_c(send, from_or_to_client, length, flags, out_s.get_produced_bytes());
        }
        LOG(LOG_INFO,
            "RemoteProgramsVirtualChannel::try_launch_application: "
                "Send to server - Client Execute PDU (2)");
        cepdu.log(LOG_INFO);

        this->send_message_to_server(length, flags, out_s.get_produced_bytes());
    }

    void sespro_ending_in_progress() override {}

    void sespro_launch_process_ended() override {}
};
