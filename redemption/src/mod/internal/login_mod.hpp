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
 *   Author(s): Christophe Grosjean, Xiaopeng Zhou, Jonathan Poelen, Meng Tan,
 *              Jennifer Inthavong
 */


#pragma once

#include "configs/config_access.hpp"
#include "mod/internal/widget/login.hpp"
#include "mod/internal/widget/language_button.hpp"
#include "mod/internal/rail_mod_base.hpp"
#include "core/events.hpp"


class CopyPaste;

using LoginModVariables = vcfg::variables<
    vcfg::var<cfg::context::password,                   vcfg::accessmode::set>,
    vcfg::var<cfg::globals::auth_user,                  vcfg::accessmode::set>,
    vcfg::var<cfg::context::selector,                   vcfg::accessmode::ask>,
    vcfg::var<cfg::context::target_protocol,            vcfg::accessmode::ask>,
    vcfg::var<cfg::globals::target_device,              vcfg::accessmode::ask>,
    vcfg::var<cfg::globals::target_user,                vcfg::accessmode::ask>,
    vcfg::var<cfg::context::opt_message,                vcfg::accessmode::get>,
    vcfg::var<cfg::context::login_message,              vcfg::accessmode::get>,
    vcfg::var<cfg::globals::authentication_timeout,     vcfg::accessmode::get>,
    vcfg::var<cfg::internal_mod::enable_target_field,   vcfg::accessmode::get>,
    vcfg::var<cfg::internal_mod::keyboard_layout_proposals, vcfg::accessmode::get>
>;


class LoginMod : public RailInternalModBase
{
public:
    LoginMod(
        LoginModVariables vars,
        EventContainer& events,
        chars_view username, chars_view password,
        gdi::GraphicApi & drawable,
        FrontAPI & front, uint16_t width, uint16_t height,
        Rect const widget_rect, ClientExecute & rail_client_execute, Font const& font,
        Theme const& theme, CopyPaste& copy_paste, Translator tr
    );

    void move_size_widget(int16_t left, int16_t top, uint16_t width, uint16_t height) override
    {
        this->login.move_size_widget(left, top, width, height);
    }

    void acl_update(AclFieldMask const&/* acl_fields*/) override
    {}

private:
    EventsGuard events_guard;

    LanguageButton language_button;
    WidgetLogin login;

    LoginModVariables vars;
};
