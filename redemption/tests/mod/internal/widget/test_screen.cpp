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
#include "test_only/test_framework/check_img.hpp"

#include "mod/internal/widget/button.hpp"
#include "mod/internal/widget/screen.hpp"
#include "keyboard/keymap.hpp"
#include "keyboard/keylayouts.hpp"
#include "utils/theme.hpp"

#include "test_only/gdi/test_graphic.hpp"
#include "test_only/core/font.hpp"

#include "test_only/mod/internal/widget/notify_trace.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/widget/screen/"


RED_AUTO_TEST_CASE(TestScreenEvent)
{
    TestGraphic drawable(300, 200);

    WidgetScreen wscreen(drawable, drawable.width(), drawable.height(), global_font_deja_vu_14(), Theme{});

    wscreen.rdp_input_invalidate(wscreen.get_rect());
    NotifyTrace notifier1;
    NotifyTrace notifier2;
    NotifyTrace notifier3;
    NotifyTrace notifier4;

    WidgetButton::Colors colors{
        .focus = {
            .fg = NamedBGRColor::WHITE,
            .bg = NamedBGRColor::WINBLUE,
            .border = NamedBGRColor::WHITE,
        },
        .blur = {
            .fg = NamedBGRColor::WHITE,
            .bg = NamedBGRColor::BG_BLUE,
            .border = NamedBGRColor::WHITE,
        },
    };

    WidgetButton wbutton1(
        drawable, global_font_deja_vu_14(), "button 1"_av, colors, notifier1);
    wbutton1.set_xy(0, 0);

    WidgetButton wbutton2(
        drawable, global_font_deja_vu_14(), "button 2"_av, colors, notifier2);
    wbutton2.set_xy(0, 30);

    WidgetButton wbutton3(
        drawable, global_font_deja_vu_14(), "button 3"_av, colors, notifier3);
    wbutton3.set_xy(100, 0);

    WidgetButton wbutton4(
        drawable, global_font_deja_vu_14(), "button 4"_av, colors, notifier4);
    wbutton4.set_xy(100, 30);

    wscreen.add_widget(wbutton1);
    wscreen.add_widget(wbutton2, WidgetComposite::HasFocus::Yes);
    wscreen.add_widget(wbutton3);
    wscreen.add_widget(wbutton4);

    wscreen.init_focus();

    wscreen.rdp_input_invalidate(wscreen.get_rect());

    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_1.png");


    Keymap keymap(*find_layout_by_id(KeyLayout::KbdId(0x040C)));

    auto rdp_input_scancode = [&](Keymap::KeyCode keycode, Keymap::KbdFlags flags = Keymap::KbdFlags()){
        auto ukeycode = underlying_cast(keycode);
        auto scancode = Keymap::Scancode(ukeycode & 0x7F);
        flags |= (ukeycode & 0x80) ? Keymap::KbdFlags::Extended : Keymap::KbdFlags();
        keymap.event(flags, scancode);
        wscreen.rdp_input_scancode(flags, scancode, 0, keymap);
    };

    rdp_input_scancode(Keymap::KeyCode::Tab);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_2.png");

    rdp_input_scancode(Keymap::KeyCode::Tab);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_3.png");

    rdp_input_scancode(Keymap::KeyCode::Tab);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_4.png");

    rdp_input_scancode(Keymap::KeyCode::LShift);
    rdp_input_scancode(Keymap::KeyCode::Tab);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_3.png");

    rdp_input_scancode(Keymap::KeyCode::Tab);
    rdp_input_scancode(Keymap::KeyCode::LShift, Keymap::KbdFlags::Release);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_2.png");

    wscreen.rdp_input_mouse(MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN,
                            wbutton1.x(), wbutton1.y());
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_7.png");

    wscreen.rdp_input_mouse(MOUSE_FLAG_BUTTON1,
                            wbutton2.x(), wbutton2.y());
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_4.png");

    rdp_input_scancode(Keymap::KeyCode::Tab);
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_1.png");

    wscreen.rdp_input_mouse(MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN,
                            wbutton4.x(), wbutton4.y());
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 0);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_10.png");

    wscreen.rdp_input_mouse(MOUSE_FLAG_BUTTON1,
                            wbutton4.x(), wbutton4.y());
    RED_CHECK(notifier1.get_and_reset() == 0);
    RED_CHECK(notifier2.get_and_reset() == 0);
    RED_CHECK(notifier3.get_and_reset() == 0);
    RED_CHECK(notifier4.get_and_reset() == 1);
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_3.png");

    /*
     * Tooltip
     */

    wscreen.show_tooltip("tooltip test"_av, 30, 35, Rect(0, 0, 500, 41), wscreen.get_rect());

    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_12.png");

    wscreen.show_tooltip(nullptr, 30, 35, Rect(), Rect());  // or hide_tooltip()
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_3.png");

    wscreen.show_tooltip(
        "Test tooltip\n"
        "Text modification\n"
        "text has been changed !"_av,
        72, 42, Rect(0, 0, 400, 80), Rect()
    );
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_13.png");

    wscreen.hide_tooltip();
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "screen_3.png");
}
