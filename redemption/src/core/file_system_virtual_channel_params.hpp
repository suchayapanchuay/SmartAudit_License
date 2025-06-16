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
    Copyright (C) Wallix 2019
    Author(s): Christophe Grosjean
*/

#pragma once

struct FileSystemVirtualChannelParams {

    bool file_system_read_authorized  = false;
    bool file_system_write_authorized = false;

    bool parallel_port_authorized     = false;
    bool print_authorized             = false;
    bool serial_port_authorized       = false;
    bool smart_card_authorized        = false;

    bool smartcard_passthrough        = false;

    explicit FileSystemVirtualChannelParams() = default;
};
