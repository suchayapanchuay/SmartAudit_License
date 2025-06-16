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
  Copyright (C) Wallix 2010
  Author(s): Christophe Grosjean, Javier Caverni, Dominique Lafages,
             Raphael Zhou, Meng Tan, Clément Moroldo
  Based on xrdp Copyright (C) Jay Sorg 2004-2010

  rdp module main header file
*/

#pragma once

#include <cstdint>
#include <climits>
#include <string>

#include "utils/static_string.hpp"
#include "utils/sugar/bounded_array_view.hpp"


struct RdpNegociationResult
{
    uint16_t front_width = 0;
    uint16_t front_height = 0;
    bool use_rdp5 = true;
    uint16_t userid = 0;
    uint32_t encryptionLevel = 0;
    uint32_t encryptionMethod = 0;
};

struct RdpLogonInfo
{
    RdpLogonInfo(bounded_chars_view<0, HOST_NAME_MAX> hostname, bool hide_client_name,
                 char const* target_user, bool split_domain) noexcept;

    [[nodiscard]] std::string const& username() const noexcept { return this->_username; }
    [[nodiscard]] std::string const& domain() const noexcept { return this->_domain; }
    [[nodiscard]] static_string<HOST_NAME_MAX> const& hostname() const noexcept { return this->_hostname; }

    [[nodiscard]] bool client_name_is_hidden() const noexcept { return this->hide_client_name; }

private:
    bool const hide_client_name;

    std::string _username;
    std::string _domain;
    static_string<HOST_NAME_MAX> _hostname;
};
