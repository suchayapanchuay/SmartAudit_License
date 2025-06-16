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

#include "core/channel_list.hpp"
#include "core/dynamic_channels_authorizations.hpp"
#include "core/RDP/channels/drdynvc.hpp"
#include "mod/rdp/channels/base_channel.hpp"
#include "utils/log.hpp"
#include "core/stream_throw_helpers.hpp"

struct DynamicChannelVirtualChannelParam
{
    zstring_view allowed_channels = "*"_zv;
    zstring_view denied_channels  = ""_zv;
};

class DynamicChannelVirtualChannel final : public BaseVirtualChannel
{
    DynamicChannelsAuthorizations dynamic_channels_authorizations;
    SessionLogApi& session_log;
    RDPVerbose verbose;

public:
    explicit DynamicChannelVirtualChannel(
        VirtualChannelDataSender* to_client_sender_,
        VirtualChannelDataSender* to_server_sender_,
        const DynamicChannelVirtualChannelParam & params,
        SessionLogApi& session_log,
        RDPVerbose verbose)
    : BaseVirtualChannel(to_client_sender_, to_server_sender_)
    , dynamic_channels_authorizations(params.allowed_channels, params.denied_channels)
    , session_log(session_log)
    , verbose(verbose)
    {}

    void process_client_message(uint32_t total_length,
        uint32_t flags, bytes_view chunk_data) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::drdynvc), LOG_INFO,
            "DynamicChannelVirtualChannel::process_client_message: "
                "total_length=%u flags=0x%08X chunk_data_length=%zu",
            total_length, flags, chunk_data.size());

        if (bool(this->verbose & RDPVerbose::drdynvc_dump)) {
            const bool send              = false;
            const bool from_or_to_client = true;
            ::msgdump_c(send, from_or_to_client, total_length, flags, chunk_data);
        }

        uint8_t Cmd = 0x00;

        if (flags & CHANNELS::CHANNEL_FLAG_FIRST) {
            InStream chunk(chunk_data);

            /* cbId(:2) + Sp(:2) + Cmd(:4) */
            ::check_throw(chunk, 1, "DynamicChannelVirtualChannel::process_client_message", ERR_RDP_DATA_TRUNCATED);

            Cmd = ((chunk.in_uint8() & 0xF0) >> 4);
        }

        InStream chunk(chunk_data);

        bool send_message_to_server = true;

        switch (Cmd)
        {
            // case drdynvc::CMD_CAPABILITIES:
            // break;

            case drdynvc::CMD_CREATE:
                if (bool(this->verbose & RDPVerbose::drdynvc) &&
                    ((flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) == (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)))
                {
                    drdynvc::DVCCreateResponsePDU create_response;

                    create_response.receive(chunk);

                    LOG(LOG_INFO,
                        "FileSystemVirtualChannel::process_client_message:");
                    create_response.log(LOG_INFO);
                }
            break;

            default:
                // assert(false);

                LOG_IF(bool(this->verbose & RDPVerbose::drdynvc), LOG_INFO,
                    "DynamicChannelVirtualChannel::process_client_message: "
                        "Delivering unprocessed messages %s(%u) to server.",
                    drdynvc::get_DCVC_Cmd_name(Cmd),
                    static_cast<unsigned>(Cmd));
            break;
        }   // switch (Cmd)

        if (send_message_to_server) {
            this->send_message_to_server(total_length, flags, chunk_data);
        }
    }   // process_client_message

    void process_server_message(uint32_t total_length,
        uint32_t flags, bytes_view chunk_data) override
    {
        LOG_IF(bool(this->verbose & RDPVerbose::drdynvc), LOG_INFO,
            "DynamicChannelVirtualChannel::process_server_message: "
                "total_length=%u flags=0x%08X chunk_data_length=%zu",
            total_length, flags, chunk_data.size());

        if (bool(this->verbose & RDPVerbose::drdynvc_dump)) {
            const bool send              = false;
            const bool from_or_to_client = false;
            ::msgdump_c(send, from_or_to_client, total_length, flags, chunk_data);
        }

        if (flags && !(flags &~ (CHANNELS::CHANNEL_FLAG_SUSPEND | CHANNELS::CHANNEL_FLAG_RESUME))) {
            this->send_message_to_client(total_length, flags, chunk_data);
            return;
        }

        uint8_t Cmd = 0x00;

        if (flags & CHANNELS::CHANNEL_FLAG_FIRST) {
            InStream chunk(chunk_data);

            /* cbId(:2) + Sp(:2) + Cmd(:4) */
            ::check_throw(chunk, 1, "DynamicChannelVirtualChannel::process_server_message", ERR_RDP_DATA_TRUNCATED);

            Cmd = ((chunk.in_uint8() & 0xF0) >> 4);
        }

        InStream chunk(chunk_data);

        bool send_message_to_client = true;

        switch (Cmd)
        {
            case drdynvc::CMD_CREATE:
                if ((flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) ==
                    (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST))
                {
                    drdynvc::DVCCreateRequestPDU create_request;

                    create_request.receive(chunk);

                    if (bool(this->verbose & RDPVerbose::drdynvc))
                    {
                        LOG(LOG_INFO,
                            "FileSystemVirtualChannel::process_server_message:");
                        create_request.log(LOG_INFO);
                    }

                    zstring_view channel_name = create_request.ChannelName();

                    bool const is_authorized = this->dynamic_channels_authorizations.is_authorized(channel_name.to_sv());
                    if (!is_authorized)
                    {
                        uint8_t message_buffer[1024];

                        OutStream out_stream(message_buffer);

                        drdynvc::DVCCreateResponsePDU create_response(create_request.ChannelId(), 0xC0000001);

                        create_response.emit(out_stream);

                        if (bool(this->verbose & RDPVerbose::drdynvc)) {
                            LOG(LOG_INFO,
                                "FileSystemVirtualChannel::process_server_message:");
                            create_response.log(LOG_INFO);
                        }

                        this->send_message_to_server(
                            out_stream.get_offset(),
                              CHANNELS::CHANNEL_FLAG_FIRST
                            | CHANNELS::CHANNEL_FLAG_LAST,
                            out_stream.get_produced_bytes());
                    }

                    this->session_log.log6((is_authorized
                            ? LogId::DYNAMIC_CHANNEL_CREATION_ALLOWED
                            : LogId::DYNAMIC_CHANNEL_CREATION_REJECTED),
                        {KVLog("channel_name"_av, channel_name)});

                    send_message_to_client = is_authorized;
                }
            break;

            default:
                // assert(false);

                LOG_IF(bool(this->verbose & RDPVerbose::drdynvc), LOG_INFO,
                    "DynamicChannelVirtualChannel::process_server_message: "
                        "Delivering unprocessed messages %s(%u) to client.",
                    drdynvc::get_DCVC_Cmd_name(Cmd),
                    static_cast<unsigned>(Cmd));
            break;
        }   // switch (Cmd)

        if (send_message_to_client) {
            this->send_message_to_client(total_length, flags, chunk_data);
        }   // switch (this->server_message_type)
    }   // process_server_message
};
