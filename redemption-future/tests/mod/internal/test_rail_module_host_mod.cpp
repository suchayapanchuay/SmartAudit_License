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
  Copyright (C) Wallix 2020
  Author(s): Proxy Team
*/

#include "mod/internal/rail_module_host_mod.hpp"
#include "mod/null/null.hpp"
#include "utils/timebase.hpp"
#include "RAIL/client_execute.hpp"
#include "core/events.hpp"
#include "core/RDP/capabilities/window.hpp"
#include "gdi/graphic_api_forwarder.hpp"
#include "utils/theme.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"

#include "test_only/front/fake_front.hpp"
#include "test_only/core/font.hpp"

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/check_img.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/rail_module_host_mod/"

struct TestRectMod : null_mod
{
    // vector of Rect
    uint16_t width;
    uint16_t height;
    gdi::GraphicApi* gd;

    TestRectMod(uint16_t width, uint16_t height, gdi::GraphicApi& gd)
    : width(width)
    , height(height)
    , gd(&gd)
    {}

    void rdp_input_invalidate(Rect rect) override
    {
        this->gd->draw(
            RDPOpaqueRect(rect, encode_color24()(NamedBGRColor::RED)),
            rect,
            gdi::ColorCtx(gdi::Depth::depth24(), nullptr));
    }

    Dimension get_dim() const override
    {
        return {width, height};
    }
};

struct TestGd : gdi::GraphicApiForwarder<gdi::GraphicApi&>
{
    using gdi::GraphicApiForwarder<gdi::GraphicApi&>::GraphicApiForwarder;

    void cached_pointer(gdi::CachePointerIndex cache_idx) override
    {
        this->last_cursor = cache_idx.cache_index();
    }

    void new_pointer(gdi::CachePointerIndex cache_idx, RdpPointerView const& cursor) override
    {
        (void)cache_idx;
        (void)cursor;
    }

    uint16_t last_cursor = 0xFFFF;
};

RED_AUTO_TEST_CASE(TestRailHostMod)
{
    const uint16_t w = 400;
    const uint16_t h = 200;

    ScreenInfo screen_info{w, h, BitsPerPixel::BitsPP16};
    EventContainer events;
    FakeFront front(screen_info);
    WindowListCaps win_caps;
    TestGd gd(front.gd());
    ClientExecute client_execute(events.get_time_base(), gd, front, win_caps, false);
    const Theme theme;
    const GCC::UserData::CSMonitor cs_monitor;
    Font const& font = global_font_deja_vu_14();

    auto mod = std::make_unique<TestRectMod>(w, h, gdi::null_gd());
    // mod is moved in railmo
    auto& mod_ref = *mod;

    front.add_channel(channel_names::rail, 0, 0);
    client_execute.enable_remote_program(true);

    const Rect widget_rect = client_execute.adjust_rect(cs_monitor.get_widget_rect(w, h));

    RailModuleHostMod host_mod(
        events, gd, w, h, widget_rect,
        client_execute, font, theme, cs_monitor);
    client_execute.ready(host_mod, font, false);
    host_mod.set_mod(std::move(mod));

    mod_ref.gd = &host_mod.proxy_gd();

    RED_TEST(w == host_mod.get_dim().w);
    RED_TEST(h == host_mod.get_dim().h);

    host_mod.rdp_input_invalidate(Rect{ 0, 0, w, h });
    RED_CHECK_IMG(front, IMG_TEST_PATH "rail1.png");

    // set pointer mod
    mod_ref.gd->cached_pointer(PredefinedPointer::Normal);

    // move to top border
    host_mod.rdp_input_mouse(MOUSE_FLAG_MOVE, 200, 19);
    RED_TEST(gd.last_cursor == gdi::CachePointerIndex(PredefinedPointer::NS).cache_index());

    // move to widget
    host_mod.rdp_input_mouse(MOUSE_FLAG_MOVE, 200, 100);
    RED_TEST(gd.last_cursor == gdi::CachePointerIndex(PredefinedPointer::Normal).cache_index());
}
