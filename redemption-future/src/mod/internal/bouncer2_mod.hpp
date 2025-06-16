/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2011
   Author(s): Christophe Grosjean, Martin Potier, Jonathan Poelen,
              Meng Tan, Raphaël Zhou

   Bouncer test, high level API
*/

#pragma once

#include "mod/mod_api.hpp"
#include "core/events.hpp"
#include "utils/timebase.hpp"

class FrontAPI;
namespace gdi
{
    class GraphicApi;
}

class Bouncer2Mod : public mod_api
{
    uint16_t front_width;
    uint16_t front_height;

    int speedx = 2;
    int speedy = 2;

    Rect dancing_rect;

    bool draw_green_carpet = true;

    uint16_t mouse_x = 0;
    uint16_t mouse_y = 0;

    EventsGuard events_guard;
    gdi::GraphicApi & gd;

    [[nodiscard]] Rect get_screen_rect() const;

public:
    Bouncer2Mod(
         gdi::GraphicApi & gd,
         EventContainer & events,
         uint16_t width, uint16_t height);

    ~Bouncer2Mod();

    void rdp_gdi_up_and_running() override {}

    void rdp_gdi_down() override {}

    void rdp_input_invalidate(Rect /*rect*/) override
    {
        this->draw_green_carpet = true;
    }

    void rdp_input_mouse(uint16_t /*device_flags*/, uint16_t x, uint16_t y) override
    {
        this->mouse_x = x;
        this->mouse_y = y;
    }

    void rdp_input_mouse_ex(uint16_t /*device_flags*/, uint16_t x, uint16_t y) override
    {
        this->mouse_x = x;
        this->mouse_y = y;
    }

    void rdp_input_scancode(KbdFlags flags, Scancode scancode, uint32_t event_time, Keymap const& keymap) override;

    void rdp_input_unicode(KbdFlags flag, uint16_t unicode) override
    {
        (void)flag;
        (void)unicode;
    }

    void rdp_input_synchronize(KeyLocks locks) override
    {
        (void)locks;
    }

    [[nodiscard]] Dimension get_dim() const override;

public:
    // This should come from BACK!
    void draw_event(gdi::GraphicApi & gd);

    [[nodiscard]] bool is_up_and_running() const override
    {
        return true;
    }

    bool server_error_encountered() const override { return false; }

    void send_to_mod_channel(CHANNELS::ChannelNameId /*front_channel_name*/, InStream & /*chunk*/, std::size_t /*length*/, uint32_t /*flags*/) override {}

    void acl_update(AclFieldMask const&/* acl_fields*/) override {}

private:
    int interaction();
};
