/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2014
    Author(s): Christophe Grosjean, Jonathan Poelen, Raphael Zhou, Meng Tan

    Configuration file,
    parsing config file rdpproxy.ini
*/

#include "configs/config.hpp"
#include "configs/io.hpp"
#include "utils/log.hpp"
#include "utils/sugar/numerics/safe_conversions.hpp"

#include "configs/autogen/str_authid.hpp"
#include "configs/autogen/acl_and_spec_type.hpp"

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wunused-function")
#include "configs/autogen/enums_func_ini.tcc"
REDEMPTION_DIAGNOSTIC_POP()

namespace
{
    parse_error log_ini_parse_err(
        parse_error err, const char * context, char const* key, bytes_view av)
    {
        LOG_IF(err, LOG_WARNING,
            "parsing error with parameter '%s' in section [%s] for \"%.*s\": %s",
            key, context, int(av.size()), av.as_chars().data(), err.c_str());
        return err;
    }

    template<class T, class U>
    parse_error config_parse_and_log(
        const char * context, char const* key, T & x, U u, bytes_view av)
    {
        return log_ini_parse_err(parse_from_cfg(x, u, av), context, key, av);
    }

    parse_error log_acl_parse_err(
        parse_error err, zstring_view key, bytes_view value)
    {
        LOG_IF(err, LOG_WARNING,
            "parsing error with acl parameter '%s' for \"%.*s\": %s",
            key, int(value.size()), value.as_chars().data(), err.c_str());

        return err;
    }

    template<class Cfg>
    struct ConfigFieldVTable
    {
        static bool parse(configs::VariablesConfiguration & variables, bytes_view value)
        {
            return !log_acl_parse_err(
                parse_from_cfg(
                    static_cast<Cfg&>(variables).value,
                    configs::spec_type<typename configs::acl_and_spec_type<Cfg>::type>{},
                    value),
                configs::authstr[unsigned(Cfg::index)],
                value);
        }

        static zstring_view to_zstring_view(
            configs::VariablesConfiguration const& variables,
            writable_chars_view buffer)
        {
            return assign_zbuf_from_cfg(
                buffer,
                cfg_s_type<typename configs::acl_and_spec_type<Cfg>::type>{},
                static_cast<Cfg const&>(variables).value
            );
        }
    };

    template<class>
    struct ConfigFieldVTableMaker;

    template<class Cfg, class... Cfgs>
    struct ConfigFieldVTableMaker<configs::Pack<Cfg, Cfgs...>>
    {
        static constexpr auto make_parsers()
        {
            return std::array<decltype(&ConfigFieldVTable<Cfg>::parse), sizeof...(Cfgs)+1>{
                &ConfigFieldVTable<Cfg>::parse,
                &ConfigFieldVTable<Cfgs>::parse...
            };
        }

        static constexpr auto make_to_zstring_view()
        {
            return std::array<decltype(&ConfigFieldVTable<Cfg>::to_zstring_view), sizeof...(Cfgs)+1>{
                &ConfigFieldVTable<Cfg>::to_zstring_view,
                &ConfigFieldVTable<Cfgs>::to_zstring_view...
            };
        }
    };

    inline constexpr auto config_parse_value_fns
        = ConfigFieldVTableMaker<configs::VariablesAclPack>::make_parsers();

    inline constexpr auto config_to_zstring_view_fns
        = ConfigFieldVTableMaker<configs::VariablesAclPack>::make_to_zstring_view();
} // anonymous namespace

#include "configs/autogen/set_value.tcc"

zstring_view Inifile::FieldConstReference::to_zstring_view(
    Inifile::ZStringBuffer& buffer) const
{
    return config_to_zstring_view_fns[unsigned(this->id)](
        this->ini->variables, writable_chars_view(buffer));
}

zstring_view Inifile::FieldConstReference::get_acl_name() const noexcept
{
    return configs::authstr[unsigned(this->id)];
}

bool Inifile::FieldReference::parse(bytes_view value)
{
    bool const ok = config_parse_value_fns[unsigned(this->id)](this->ini->variables, value);
    if (ok) {
        this->ini->asked_table.clear(this->id);
    }
    return ok;
}

Inifile::FieldReference Inifile::get_acl_field_by_name(chars_view name) noexcept
{
    auto name_sv = std::string_view{name.data(), name.size()};
    using int_type = std::underlying_type_t<configs::authid_t>;
    for (int_type i = 0; i < int_type(configs::max_authid); ++i) {
        if (configs::authstr[i].to_sv() == name_sv) {
            return {*this, authid_t(i)};
        }
    }
    return {};
}

void Inifile::initialize()
{
    this->ask<cfg::context::target_password>();
    this->ask<cfg::context::target_host>();
    this->ask<cfg::context::target_protocol>();
    this->ask<cfg::context::password>();
    this->ask<cfg::globals::auth_user>();
    this->ask<cfg::globals::target_device>();
    this->ask<cfg::globals::target_user>();

    this->push_to_send_index<cfg::context::opt_bpp>();
    this->push_to_send_index<cfg::context::opt_width>();
    this->push_to_send_index<cfg::context::opt_height>();
    this->push_to_send_index<cfg::context::selector_current_page>();
    this->push_to_send_index<cfg::context::selector_device_filter>();
    this->push_to_send_index<cfg::context::selector_group_filter>();
    this->push_to_send_index<cfg::context::selector_proto_filter>();
    this->push_to_send_index<cfg::context::selector_lines_per_page>();
    this->push_to_send_index<cfg::context::reporting>();
    this->push_to_send_index<cfg::context::auth_channel_target>();
    this->push_to_send_index<cfg::context::accept_message>();
    this->push_to_send_index<cfg::context::display_message>();
    this->push_to_send_index<cfg::context::real_target_device>();
    this->push_to_send_index<cfg::globals::host>();
    this->push_to_send_index<cfg::globals::target>();

    this->asked_table.set(cfg::context::target_port::index);
}

namespace
{
    template<class>
    struct ResetIniValue;

    template<class T>
    void reset_value(T& dest, T&& src)
    {
        dest = static_cast<T&&>(src);
    }

    template<std::size_t N>
    void reset_value(char(&dest)[N], char(&&src)[N])
    {
        std::memcpy(dest, src, N);
    }

    template<class... Cfg>
    struct ResetIniValue<configs::Pack<Cfg...>>
    {
        static void reset(configs::VariablesConfiguration& variables)
        {
            (..., reset_value(static_cast<Cfg&>(variables).value, Cfg{}.value));
        }
    };
} // anonymous namespace

#include "configs/autogen/cfg_ini_pack.hpp"

void Inifile::reset_ini_values()
{
    ResetIniValue<configs::cfg_ini_infos::IniPack>::reset(variables);
}
