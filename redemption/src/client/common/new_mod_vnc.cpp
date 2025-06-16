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

#include "client/common/new_mod_vnc.hpp"
#include "mod/vnc/vnc.hpp"


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
)
{
    return std::make_unique<mod_vnc>(
        t, random, gd, events, username, password, front,
        front_width, front_height, clipboard_up, clipboard_down, encodings,
        mod_vnc::ClipboardEncodingType::UTF8, VncBogusClipboardInfiniteLoop::delayed,
        layout, locks, server_is_macos, send_alt_ksym,
        cursor_pseudo_encoding_supported, rail_client_execute, verbose, session_log,
        tls_config, force_authentication_method, server_cert_params, device_id
    );
}
