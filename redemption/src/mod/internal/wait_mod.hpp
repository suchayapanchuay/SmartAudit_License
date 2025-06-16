/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2014
 *   Author(s): Christophe Grosjean, Xiaopeng Zhou, Jonathan Poelen,
 *              Meng Tan, Jennifer Inthavong
 */


#pragma once

#include "configs/config_access.hpp"
#include "mod/internal/rail_mod_base.hpp"
#include "mod/internal/widget/wait.hpp"
#include "mod/internal/widget/language_button.hpp"
#include "core/events.hpp"


using WaitModVariables = vcfg::variables<
    vcfg::var<cfg::internal_mod::keyboard_layout_proposals, vcfg::accessmode::get>,
    vcfg::var<cfg::context::comment,                    vcfg::accessmode::set>,
    vcfg::var<cfg::context::duration,                   vcfg::accessmode::set>,
    vcfg::var<cfg::context::ticket,                     vcfg::accessmode::set>,
    vcfg::var<cfg::context::waitinforeturn,             vcfg::accessmode::set>,
    vcfg::var<cfg::context::duration_max,               vcfg::accessmode::get>
>;


class WaitMod : public RailInternalModBase
{
public:
    WaitMod(
        WaitModVariables vars,
        EventContainer& events,
        gdi::GraphicApi & drawable,
        FrontAPI & front,
        uint16_t width, uint16_t height, Rect const widget_rect, chars_view caption,
        chars_view message, ClientExecute & rail_client_execute, Font const& font,
        Theme const& theme, CopyPaste& copy_paste, Translator tr, uint32_t flag);

    void move_size_widget(int16_t left, int16_t top, uint16_t width, uint16_t height) override
    {
        this->wait_widget.move_size_widget(left, top, width, height);
    }

    void acl_update(AclFieldMask const&/* acl_fields*/) override {}

private:
    LanguageButton language_button;
    WidgetWait wait_widget;

    WaitModVariables vars;

    EventsGuard events_guard;

    void confirm();
    void accepted();
    void refused();
};
