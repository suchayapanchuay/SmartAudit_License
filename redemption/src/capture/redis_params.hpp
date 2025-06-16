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
Copyright (C) Wallix 2021
Author(s): Proxies Team
*/

#pragma once

#include "utils/sugar/zstring_view.hpp"

#include <chrono>

struct RedisParams
{
    struct Tls
    {
        bool enable_tls;
        const char * cert_file;
        const char * key_file;
        const char * ca_cert_file;
    };

    zstring_view address;
    unsigned port;
    zstring_view password;
    unsigned db;
    std::chrono::milliseconds timeout;
    Tls tls;
};
