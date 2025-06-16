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
Copyright (C) Wallix 2010-2019
Author(s): Jonathan Poelen
*/

#pragma once

#ifdef IN_IDE_PARSER
# define __EMSCRIPTEN__
#endif

#include "core/channel_list.hpp"
#include "core/front_api.hpp"
#include "mod/rdp/rdp_verbose.hpp"
#include "redjs/channel_receiver.hpp"
#include "redjs/graphics.hpp"

#include "emscripten/val.h"

#include <vector>
#include <string_view>

class ScreenInfo;

namespace redjs
{

class Front : public FrontAPI
{
public:
    Front(emscripten::val callbacks, uint16_t width, uint16_t height, RDPVerbose verbose);
    ~Front();

    PrimaryDrawingOrdersSupport get_supported_orders() const;

    void add_channel_receiver(ChannelReceiver channel);

    bool can_be_start_capture(SessionLogApi& session_log) override;
    bool must_be_stop_capture() override;
    bool is_capture_in_progress() const override;


    ResizeResult server_resize(ScreenInfo screen_server) override;

    CHANNELS::ChannelDefArrayView get_channel_list() const override
    {
        return cl;
    }

    void send_to_channel(
        CHANNELS::ChannelDef const& channel_def, bytes_view chunk_data,
        std::size_t total_data_len, uint32_t flags) override;

    void update_pointer_position(uint16_t x, uint16_t y) override;

    gdi::GraphicApi& graphic_api() noexcept { return this->gd; }

    void session_update(MonotonicTimePoint now, LogId id, KVLogList kv_list) override
    {
        (void)now;
        (void)id;
        (void)kv_list;
    }

    void possible_active_window_change() override {}
    void must_flush_capture() override {}

private:
    Graphics gd;
    RDPVerbose verbose;
    CHANNELS::ChannelDefArray cl;

    struct Channel
    {
        void* ctx;
        ChannelReceiver::receiver_func_t do_receive;
    };
    std::vector<Channel> channels;
};

}
