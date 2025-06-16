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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Meng Tan
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"
#include "test_only/test_framework/check_img.hpp"
#include "test_only/front/fake_front.hpp"
#include "test_only/core/font.hpp"

#include "core/RDP/capabilities/window.hpp"
#include "RAIL/client_execute.hpp"
#include "mod/internal/close_mod.hpp"
#include "keyboard/keymap.hpp"
#include "keyboard/keylayouts.hpp"
#include "configs/config.hpp"
#include "gdi/graphic_api_forwarder.hpp"
#include "utils/theme.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/close_mod/"

using namespace std::literals::chrono_literals;

RED_AUTO_TEST_CASE(TestCloseMod)
{
    ScreenInfo screen_info{800, 600, BitsPerPixel{24}};
    FakeFront front(screen_info);
    WindowListCaps window_list_caps;
    EventManager event_manager;
    auto& events = event_manager.get_events();
    ClientExecute client_execute(events.get_time_base(), front.gd(), front, window_list_caps, false);

    Theme theme;

    Keymap keymap(*find_layout_by_id(KeyLayout::KbdId(0x040C)));

    Inifile ini;

    auto& event_cont = detail::ProtectedEventContainer::get_events(events);
    {
        CloseMod d("message"_av, ini, events, front.gd(),
                   screen_info.width, screen_info.height, Rect(0, 0, 799, 599),
                   client_execute, global_font_deja_vu_14(), theme,
                   MsgTranslationCatalog::default_catalog(), false);

        keymap.event(Keymap::KbdFlags(), Keymap::Scancode(0x01)); // esc
        d.rdp_input_scancode(Keymap::KbdFlags(), Keymap::Scancode(0x01), 0, keymap);

        RED_CHECK(event_cont.size() == 2);
        event_manager.get_writable_time_base().monotonic_time = MonotonicTimePoint{62s};
        event_manager.execute_events([](int){return false;}, false);
        RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_1.png");

        RED_CHECK(event_cont.size() == 2);
        event_manager.get_writable_time_base().monotonic_time = MonotonicTimePoint{581s};
        event_manager.execute_events([](int){return false;}, false);
        RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_2.png");

        RED_CHECK(event_cont.size() == 2);
        event_manager.get_writable_time_base().monotonic_time = MonotonicTimePoint{600s};
        event_manager.execute_events([](int){return false;}, false);
        RED_CHECK(event_cont.size() == 1);

        RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_3.png");
        RED_CHECK(d.get_mod_signal() == BACK_EVENT_STOP);
    }

    // When Close mod goes out of scope remaining events should be garbaged
    RED_CHECK(event_cont.size() == 1);
    event_manager.execute_events([](int){return false;}, false);
    RED_CHECK(event_cont.size() == 0);
}

RED_AUTO_TEST_CASE(TestCloseModSelector)
{
    ScreenInfo screen_info{800, 600, BitsPerPixel{24}};
    FakeFront front(screen_info);
    WindowListCaps window_list_caps;
    EventManager event_manager;
    auto& events = event_manager.get_events();
    ClientExecute client_execute(events.get_time_base(), front.gd(), front, window_list_caps, false);
    Theme theme;

    Keymap keymap(*find_layout_by_id(KeyLayout::KbdId(0x040C)));

    Inifile ini;
    CloseMod d("message"_av, ini, events, front.gd(),
        screen_info.width, screen_info.height, Rect(0, 0, 799, 599), client_execute,
        global_font_deja_vu_14(), theme, MsgTranslationCatalog::default_catalog(), true);

    keymap.event(Keymap::KbdFlags(), Keymap::Scancode(0x01)); // esc
    d.rdp_input_scancode(Keymap::KbdFlags(), Keymap::Scancode(0x01), 0, keymap);

    event_manager.get_writable_time_base().monotonic_time = MonotonicTimePoint{2min};
    event_manager.execute_events([](int){return false;}, false);

    RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_selector_1.png");

    d.rdp_input_invalidate(d.set_back_to_selector(false));
    RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_1.png");

    d.rdp_input_invalidate(d.set_back_to_selector(true));
    RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_selector_1.png");
}

RED_AUTO_TEST_CASE(TestCloseModRail)
{
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

    ScreenInfo screen_info{800, 600, BitsPerPixel{24}};
    FakeFront front(screen_info);
    TestGd gd(front.gd());
    WindowListCaps window_list_caps;
    EventManager event_manager;
    auto& events = event_manager.get_events();
    ClientExecute client_execute(events.get_time_base(), gd, front, window_list_caps, false);

    Theme theme;

    Keymap keymap(*find_layout_by_id(KeyLayout::KbdId(0x040C)));

    front.add_channel(channel_names::rail, 0, 0);
    client_execute.enable_remote_program(true);

    const Rect widget_rect = client_execute.adjust_rect(
        Rect(0, 0, screen_info.width-1u, screen_info.height-1u));

    Inifile ini;
    CloseMod d("message"_av, ini, events, gd,
        screen_info.width, screen_info.height, widget_rect, client_execute,
        global_font_deja_vu_14(), theme, MsgTranslationCatalog::default_catalog(), true);
    client_execute.ready(d, global_font_deja_vu_14(), false);

    keymap.event(Keymap::KbdFlags(), Keymap::Scancode(0x01)); // esc
    d.rdp_input_scancode(Keymap::KbdFlags(), Keymap::Scancode(0x01), 0, keymap);

    d.rdp_input_invalidate(Rect{ 0, 0, screen_info.width, screen_info.height });

    event_manager.get_writable_time_base().monotonic_time = MonotonicTimePoint{1s};
    event_manager.execute_events([](int){return false;}, false);

    RED_CHECK_IMG(front, IMG_TEST_PATH "close_mod_rail.png");

    // set pointer mod
    gd.last_cursor = gdi::CachePointerIndex(PredefinedPointer::Normal).cache_index();

    // move to top border
    d.rdp_input_mouse(MOUSE_FLAG_MOVE, 200, 59);
    RED_TEST(gd.last_cursor == gdi::CachePointerIndex(PredefinedPointer::NS).cache_index());

    // move to widget
    d.rdp_input_mouse(MOUSE_FLAG_MOVE, 200, 100);
    RED_TEST(gd.last_cursor == gdi::CachePointerIndex(PredefinedPointer::Normal).cache_index());
}
