/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan
 */

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/check_img.hpp"
#include "test_only/gdi/test_graphic.hpp"
#include "test_only/core/font.hpp"
#include "test_only/mod/internal/widget/notify_trace.hpp"

#include "mod/internal/widget/wab_close.hpp"
#include "utils/theme.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/widget/wab_close/"

struct TestWidgetCloseCtx
{
    TestGraphic drawable{800, 600};
    WidgetWabClose flat_wab_close;

    TestWidgetCloseCtx(
        std::string diagnostic_text, chars_view username, chars_view target,
        bool showtimer, Theme theme = Theme(),
        WidgetEventNotifier oncancel = WidgetEventNotifier())
    : flat_wab_close(
        drawable, 0, 0, 800, 600, {oncancel, WidgetEventNotifier()},
        std::move(diagnostic_text), username, target,
        showtimer, global_font_deja_vu_14(), theme,
        MsgTranslationCatalog::default_catalog(), false)
    {
        flat_wab_close.init_focus();
    }
};

RED_AUTO_TEST_CASE(TraceWidgetWabClose)
{
    TestWidgetCloseCtx ctx("abc\ndef", "rec"_av, "rec"_av, false);

    ctx.flat_wab_close.rdp_input_invalidate(ctx.flat_wab_close.get_rect());

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_1.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWabClose2)
{
    TestWidgetCloseCtx ctx(
        "Lorem ipsum dolor sit amet, consectetur\n"
        "adipiscing elit. Nam purus lacus, luctus sit\n"
        "amet suscipit vel, posuere quis turpis. Sed\n"
        "venenatis rutrum sem ac posuere. Phasellus\n"
        "feugiat dui eu mauris adipiscing sodales.\n"
        "Mauris rutrum molestie purus, in tempor lacus\n"
        "tincidunt et. Sed eu ligula mauris, a rutrum\n"
        "est. Vestibulum in nunc vel massa condimentum\n"
        "iaculis nec in arcu. Pellentesque accumsan,\n"
        "quam sit amet aliquam mattis, odio purus\n"
        "porttitor tortor, sit amet tincidunt odio\n"
        "erat ut ligula. Fusce sit amet mauris neque.\n"
        "Sed orci augue, luctus in ornare sed,\n"
        "adipiscing et arcu.",
        nullptr, nullptr, false);

    ctx.flat_wab_close.rdp_input_invalidate(ctx.flat_wab_close.get_rect());

    // ask to widget to redraw at it's current position

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_2.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWabClose3)
{
    TestWidgetCloseCtx ctx("abc\ndef", nullptr, nullptr, false);

    ctx.flat_wab_close.rdp_input_invalidate(ctx.flat_wab_close.get_rect());

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_3.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWabCloseClip)
{
    TestWidgetCloseCtx ctx("abc\ndef", nullptr, nullptr, false);

    ctx.flat_wab_close.rdp_input_invalidate(ctx.flat_wab_close.get_rect().offset(20,0));

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_4.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWabCloseClip2)
{
    TestWidgetCloseCtx ctx("abc\ndef", nullptr, nullptr, false);

    ctx.flat_wab_close.rdp_input_invalidate(Rect(
        20 + ctx.flat_wab_close.x(),
        5 + ctx.flat_wab_close.y(),
        30,
        10
    ));

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_5.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWabCloseExit)
{
    NotifyTrace notifier;
    TestWidgetCloseCtx ctx("abc\ndef", "tartempion"_av, "caufield"_av, true, Theme(), notifier);

    ctx.flat_wab_close.rdp_input_invalidate(ctx.flat_wab_close.get_rect());

    ctx.flat_wab_close.refresh_timeleft(std::chrono::seconds(183));

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_7.png");

    ctx.flat_wab_close.refresh_timeleft(std::chrono::seconds(49));

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_8.png");

    // click on cancel
    ctx.flat_wab_close.rdp_input_mouse(MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN, 373, 402);
    ctx.flat_wab_close.rdp_input_mouse(MOUSE_FLAG_BUTTON1, 373, 402);

    RED_CHECK(notifier.get_and_reset() == 1);

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wab_close_8.png");
}
