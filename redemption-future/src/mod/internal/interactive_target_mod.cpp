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

#include "configs/config.hpp"
#include "mod/internal/interactive_target_mod.hpp"
#include "translation/translation.hpp"
#include "translation/trkeys.hpp"
#include "utils/sugar/to_sv.hpp"


InteractiveTargetMod::InteractiveTargetMod(
    InteractiveTargetModVariables vars,
    gdi::GraphicApi & drawable,
    FrontAPI & front, uint16_t width, uint16_t height, Rect const widget_rect,
    ClientExecute & rail_client_execute, Font const& font, Theme const& theme,
    CopyPaste& copy_paste, Translator tr)
    : RailInternalModBase(drawable, width, height, rail_client_execute, font, theme, &copy_paste)
    , ask_device(vars.is_asked<cfg::context::target_host>())
    , ask_login(vars.is_asked<cfg::globals::target_user>())
    , ask_password((this->ask_login || vars.is_asked<cfg::context::target_password>()))
    , language_button(vars.get<cfg::internal_mod::keyboard_layout_proposals>(), this->challenge,
        drawable, front, font, LanguageButton::Colors::from_theme(theme))
    , challenge(
        drawable, copy_paste, widget_rect.x, widget_rect.y, widget_rect.cx, widget_rect.cy,
        {
            .onsubmit = [this]{ this->accepted(); },
            .oncancel = [this]{ this->refused(); },
            .onctrl_shift = [this]{ this->language_button.next_layout(); },
        },
        this->ask_device, this->ask_login, this->ask_password,
        theme,
        tr(trkeys::target_info_required),
        tr(trkeys::device), vars.get<cfg::globals::target_device>(),
        tr(trkeys::login), vars.get<cfg::globals::target_user>(),
        tr(trkeys::password),
        font, &this->language_button)
    , vars(vars)
{
    this->screen.add_widget(this->challenge, WidgetComposite::HasFocus::Yes);
    this->screen.init_focus();
    this->screen.rdp_input_invalidate(this->screen.get_rect());
}

// TODO ugly. The value should be pulled by authentifier when module is closed instead of being pushed to it by mod
void InteractiveTargetMod::accepted()
{
    if (this->ask_device) {
        this->vars.set_acl<cfg::context::target_host>(to_sv(this->challenge.get_edit_device()));
    }
    if (this->ask_login) {
        this->vars.set_acl<cfg::globals::target_user>(to_sv(this->challenge.get_edit_login()));
    }
    if (this->ask_password) {
        this->vars.set_acl<cfg::context::target_password>(to_sv(this->challenge.get_edit_password()));
    }
    this->vars.set_acl<cfg::context::display_message>(true);
    this->set_mod_signal(BACK_EVENT_NEXT);
    // throw Error(ERR_BACK_EVENT_NEXT);
}

// TODO ugly. The value should be pulled by authentifier when module is closed instead of being pushed to it by mod
void InteractiveTargetMod::refused()
{
    this->vars.set_acl<cfg::context::target_password>("");
    this->vars.set_acl<cfg::context::display_message>(false);
    this->set_mod_signal(BACK_EVENT_NEXT);
    // throw Error(ERR_BACK_EVENT_NEXT);
}
