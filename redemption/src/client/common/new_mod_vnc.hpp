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
Copyright (C) Wallix 2010-2018
Author(s): Jonathan Poelen
*/

#pragma once

#include "mod/mod_api.hpp"
#include "mod/vnc/vnc_verbose.hpp"
#include "core/events.hpp"
#include "utils/timebase.hpp"

#include <memory>
#include <string_view>

class ClientExecute;
class FrontAPI;
class Transport;
class AuthApi;
class SessionLogApi;
class KeyLayout;
class Random;
class ServerCertParams;
class TlsConfig;

namespace gdi
{
    class GraphicApi;
}

namespace kbdtypes
{
    enum class KeyLocks : uint8_t;
}

std::unique_ptr<mod_api> new_mod_vnc(
    Transport& t,
    gdi::GraphicApi & gd,
    EventContainer & events,
    SessionLogApi& session_log,
    const char* username,
    const char* password,
    FrontAPI& front,
    uint16_t front_width,
    uint16_t front_height,
    Random& random,
    bool clipboard_up,
    bool clipboard_down,
    const char * encodings,
    KeyLayout const& layout,
    kbdtypes::KeyLocks locks,
    bool server_is_macos,
    bool send_alt_ksym,
    bool cursor_pseudo_encoding_supported,
    ClientExecute* rail_client_execute,
    VNCVerbose verbose,
    TlsConfig const& tls_config,
    std::string_view force_authentication_method,
    ServerCertParams const& server_cert_params,
    std::string_view device_id
);
