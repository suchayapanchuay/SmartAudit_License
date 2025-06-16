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
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
              Olivier Hervieu, Martin Potier, Jonathan Poelen, Raphaël Zhou
              Meng Tan

   version number

*/

#pragma once

#include "main/version.hpp"
#include "utils/pp.hpp"

#include "utils/sugar/zstring_view.hpp"

inline zstring_view redemption_info_copyright() noexcept
{
    return "Copyright (C) Wallix 2010-2024"_zv;
}

inline zstring_view redemption_info_version() noexcept
{
    return "ReDemPtion " VERSION
        " ("
            RED_PP_STRINGIFY(REDEMPTION_COMP_NAME) " "
            REDEMPTION_COMP_STRING_VERSION
        ")"
    #ifndef NDEBUG
        " (DEBUG)"
    #endif
    #if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__ == 1
        " (-fsanitize=address)"
    #elif defined(__has_feature)
        #  if __has_feature(address_sanitizer)
        " (-fsanitize=address)"
        #  endif
    #endif
        ""_zv
    ;
}
