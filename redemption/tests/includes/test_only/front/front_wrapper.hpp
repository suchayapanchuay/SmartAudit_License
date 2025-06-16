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

#include "core/front_api.hpp"
#include "core/events.hpp"

class Transport;
class Random;
class Inifile;
class Callback;

namespace gdi
{
    class GraphicApi;
}

class FrontWrapper : public FrontAPI
{
public:
    FrontWrapper(
        EventContainer& events,
        AclReportApi& acl_report,
        Transport & trans,
        Random & gen,
        Inifile & ini
    );

    ~FrontWrapper();

    bool can_be_start_capture(SessionLogApi & /*session_log*/) override { return false; }
    bool must_be_stop_capture() override { return false; }
    bool is_capture_in_progress() const override { return false; }
    void must_flush_capture() override {}

    CHANNELS::ChannelDefArrayView get_channel_list() const override;

    void send_to_channel( const CHANNELS::ChannelDef & channel, bytes_view chunk_data
                        , std::size_t total_length, uint32_t flags) override;

    ResizeResult server_resize(ScreenInfo screen_server) override;

    void update_pointer_position(uint16_t x, uint16_t y) override;

    void set_keyboard_indicators(kbdtypes::KeyLocks key_locks) override;

    void session_probe_started(bool /*unused*/) override;
    void set_keylayout(KeyLayout const& keylayout) override;
    void set_focus_on_password_textbox(bool /*unused*/) override;
    void set_focus_on_unidentified_input_field(bool /*unused*/) override;
    void set_consent_ui_visible(bool /*unused*/) override;
    void session_update(MonotonicTimePoint now, LogId id, KVLogList kv_list) override;
    void possible_active_window_change() override;
    void send_savesessioninfo() override;
    KeyLayout const& get_keylayout() const override;

    gdi::GraphicApi& gd() noexcept;
    operator gdi::GraphicApi& () noexcept { return this->gd(); }

    bool is_up_and_running() const;
    void incoming(Callback & cb);
    ScreenInfo const& screen_info() const;

private:
    class D;
    D* d;
};
