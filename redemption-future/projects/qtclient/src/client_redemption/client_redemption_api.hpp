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

#pragma once

#include "core/front_api.hpp"

#include <chrono>
#include <string>
#include <ctime>

class mod_api;

class ClientRedemptionAPI : public FrontAPI
{
public:
    virtual ~ClientRedemptionAPI() = default;

    bool can_be_start_capture(SessionLogApi & /*session_log*/) override { return true; }
    bool is_capture_in_progress() const override { return true; }
    void send_to_channel( const CHANNELS::ChannelDef &  /*channel*/, bytes_view /*chunk_data*/
                        , std::size_t /*total_length*/, uint32_t /*flags*/) override {}

    // CONTROLLER
    virtual void close() = 0;
    virtual void connect(const std::string& /*ip*/, const std::string& /*name*/, const std::string& /*pwd*/, const int /*port*/) {}
    virtual void disconnect(std::string const & /*unused*/, bool /*unused*/) {}
    virtual void update_keylayout() {}

    // Replay functions
    virtual void replay( const std::string & /*unused*/) {}
    virtual bool load_replay_mod(MonotonicTimePoint begin, MonotonicTimePoint end) { (void)begin; (void)end; return false; }
    virtual void delete_replay_mod() {}
};
