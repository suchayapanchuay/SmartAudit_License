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
 *              Meng Tan, Jennifer Inthavong
 */

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/check_img.hpp"

#include "test_only/gdi/test_graphic.hpp"
#include "test_only/core/font.hpp"

#include "mod/internal/copy_paste.hpp"
#include "mod/internal/widget/wait.hpp"
#include "utils/theme.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/widget/wait/"

struct TestWidgetWaitCtx
{
    TestGraphic drawable{800, 600};
    CopyPaste copy_paste{false};
    WidgetWait flat_dialog;

    TestWidgetWaitCtx(unsigned flags = WidgetWait::NONE)
    : flat_dialog(
        drawable, copy_paste, {0, 0, 800, 600},
        {WidgetEventNotifier(), WidgetEventNotifier(), WidgetEventNotifier(), WidgetEventNotifier()},
        "Invalid Target"_av,
        "Target \"trucmuch@machinbidule:serv\" is not allowed because you either\n"
        "has no right to access it or it does not exist.\n"
        "you can either return to selector or exit."_av,
        nullptr, global_font_deja_vu_14(), []{
            Theme colors;
            colors.global.bgcolor = NamedBGRColor::BG_BLUE;
            colors.global.fgcolor = NamedBGRColor::WHITE;
            return colors;
        }(), MsgTranslationCatalog::default_catalog(), flags,
        std::chrono::minutes::zero()
    )
    {
        flat_dialog.rdp_input_invalidate(flat_dialog.get_rect());
    }
};

RED_AUTO_TEST_CASE(TraceWidgetWait)
{
    TestWidgetWaitCtx ctx;
    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wait_1.png");
}

RED_AUTO_TEST_CASE(TraceWidgetWaitWithForm)
{
    TestWidgetWaitCtx ctx(
        WidgetWait::COMMENT_DISPLAY | WidgetWait::COMMENT_MANDATORY |
        WidgetWait::TICKET_DISPLAY | WidgetWait::TICKET_MANDATORY |
        WidgetWait::DURATION_DISPLAY);

    RED_CHECK_IMG(ctx.drawable, IMG_TEST_PATH "wait_2.png");
}
