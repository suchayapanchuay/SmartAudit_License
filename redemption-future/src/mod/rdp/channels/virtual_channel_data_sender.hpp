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

#include "utils/log.hpp"
#include "utils/hexdump.hpp"

inline void msgdump_impl(
    bool from_or_to_client,
    uint32_t total_length, uint32_t flags,
    bytes_view chunk_data)
{
    const unsigned dest = (from_or_to_client
                            ? 0  // Client
                            : 1  // Server
                            );
    LOG(LOG_INFO, "\\x00\\x00\\x00\\x0%u", dest);
    LOG(LOG_INFO, "\\x%02x\\x%02x\\x%02x\\x%02x",
        (total_length >> 24), (total_length >> 16) & 0xffu,
        (total_length >> 8) & 0xffu, (total_length & 0xffu));
    LOG(LOG_INFO, "\\x%02x\\x%02x\\x%02x\\x%02x",
        (flags >> 24), (flags >> 16) & 0xff,
        (flags >> 8) & 0xff, (flags & 0xff));
    LOG(LOG_INFO, "\\x%02lx\\x%02lx\\x%02lx\\x%02lx",
        (chunk_data.size() >> 24), (chunk_data.size() >> 16) & 0xffu,
        (chunk_data.size() >> 8) & 0xffu, (chunk_data.size() & 0xffu));
    hexdump_c(chunk_data);
}

inline static void msgdump_c(bool send, bool from_or_to_client,
    uint32_t total_length, uint32_t flags, bytes_view chunk_data)
{
    if (send) {
        LOG(LOG_INFO, "Sending on channel (%zu) n bytes", chunk_data.size());
    }
    else {
        LOG(LOG_INFO, "Recv done on channel (%zu) n bytes", chunk_data.size());
    }
    msgdump_impl(from_or_to_client, total_length, flags, chunk_data);
    if (send) {
        LOG(LOG_INFO, "Sent dumped on channel (%zu) n bytes", chunk_data.size());
    }
    else {
        LOG(LOG_INFO, "Dump done on channel (%zu) n bytes", chunk_data.size());
    }
}

inline static void msgdump_d(bool send, bool from_or_to_client,
    uint32_t total_length, uint32_t flags, bytes_view chunk_data)
{
    if (send) {
        LOG(LOG_INFO, "Sending on channel (%u) n bytes", total_length);
    }
    else {
        LOG(LOG_INFO, "Recv done on channel (%u) n bytes", total_length);
    }
    msgdump_impl(from_or_to_client, total_length, flags, chunk_data);
    if (total_length > chunk_data.size()) {
        LOG(LOG_INFO, "/* ---- */ \"...                                                             \" //................");
    }
    if (send) {
        LOG(LOG_INFO, "Sent dumped on channel (%u) n bytes", total_length);
    }
    else {
        LOG(LOG_INFO, "Dump done on channel (%u) n bytes", total_length);
    }
}

class VirtualChannelDataSender
{
public:
    virtual ~VirtualChannelDataSender() = default;

    virtual VirtualChannelDataSender& SynchronousSender() {
        return *this;
    }

    // TODO add names function
    virtual void operator()(uint32_t total_length, uint32_t flags, bytes_view chunk_data) = 0;
};
