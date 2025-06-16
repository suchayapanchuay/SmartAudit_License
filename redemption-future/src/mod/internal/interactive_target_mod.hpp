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
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Xiaopeng Zhou, Jonathan Poelen,
 *              Meng Tan, Jennifer Inthavong
 */

#pragma once

#include "configs/config_access.hpp"
#include "translation/translation.hpp"
#include "mod/internal/copy_paste.hpp"
#include "mod/internal/rail_mod_base.hpp"
#include "mod/internal/widget/language_button.hpp"
#include "mod/internal/widget/interactive_target.hpp"


class CopyPaste;

using InteractiveTargetModVariables = vcfg::variables<
    vcfg::var<cfg::globals::target_user,                vcfg::accessmode::is_asked | vcfg::accessmode::set | vcfg::accessmode::get>,
    vcfg::var<cfg::context::target_password,            vcfg::accessmode::is_asked | vcfg::accessmode::set>,
    vcfg::var<cfg::context::target_host,                vcfg::accessmode::is_asked | vcfg::accessmode::set>,
    vcfg::var<cfg::globals::target_device,              vcfg::accessmode::get>,
    vcfg::var<cfg::context::display_message,            vcfg::accessmode::set>,
    vcfg::var<cfg::internal_mod::keyboard_layout_proposals, vcfg::accessmode::get>
>;


class InteractiveTargetMod : public RailInternalModBase
{
public:
    InteractiveTargetMod(
        InteractiveTargetModVariables vars,
        gdi::GraphicApi & drawable,
        FrontAPI & front, uint16_t width, uint16_t height, Rect const widget_rect,
        ClientExecute & rail_client_execute, Font const& font, Theme const& theme,
        CopyPaste& copy_paste, Translator tr);

    void move_size_widget(int16_t left, int16_t top, uint16_t width, uint16_t height) override
    {
        this->challenge.move_size_widget(left, top, width, height);
    }

    void acl_update(AclFieldMask const&/* acl_fields*/) override {}

private:
    void accepted();

    void refused();

    bool ask_device;
    bool ask_login;
    bool ask_password;

    LanguageButton language_button;
    WidgetInteractiveTarget challenge;

    InteractiveTargetModVariables vars;
};
