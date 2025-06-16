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

#include "acl/module_manager/create_module_rail.hpp"
#include "mod/internal/rail_module_host_mod.hpp"
#include "RAIL/client_execute.hpp"
#include "core/client_info.hpp"
#include "configs/config.hpp"
#include "utils/strutils.hpp"


RailModuleHostMod* create_mod_rail(
    Inifile& ini,
    EventContainer& events,
    gdi::GraphicApi & drawable,
    ClientInfo const& client_info,
    ClientExecute& rail_client_execute,
    Font const& font,
    Theme const& theme)
{
    LOG(LOG_INFO, "Creation of internal module 'RailModuleHostMod'");

    rail_client_execute.set_target_info(
        ini.get<cfg::context::target_str>(),
        ini.get<cfg::globals::primary_user_id>()
    );

    struct RailMod final : RailModuleHostMod
    {
        using RailModuleHostMod::RailModuleHostMod;

        void set_ini(Inifile& ini)
        {
            this->ini = &ini;
            this->ini->set<cfg::context::rail_module_host_mod_is_active>(true);
        }

        ~RailMod()
        {
            this->ini->set<cfg::context::rail_module_host_mod_is_active>(false);
        }

    private:
        Inifile* ini;
    };

    auto* host_mod = new RailMod(
        events,
        drawable,
        client_info.screen_info.width,
        client_info.screen_info.height,
        rail_client_execute.adjust_rect(client_info.get_widget_rect()),
        rail_client_execute,
        font,
        theme,
        client_info.cs_monitor
    );
    host_mod->set_ini(ini);

    return host_mod;
}
