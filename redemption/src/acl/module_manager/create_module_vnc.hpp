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
  Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
             Raphael Zhou, Meng Tan

  Manage Modules Life cycle : creation, destruction and chaining
  find out the next module to run from context reading
*/

#pragma once

#include "acl/mod_pack.hpp"
#include "utils/ref.hpp"

class Inifile;
class FrontAPI;
class ClientInfo;
class ClientExecute;
class Font;
class Random;
class Theme;
class EventContainer;
class SessionLogApi;
class ErrorMessageCtx;
class KeyLayout;

namespace kbdtypes
{
    enum class KeyLocks : uint8_t;
}

namespace gdi
{
    class GraphicApi;
}

ModPack create_mod_vnc(
    gdi::GraphicApi & drawable,
    Inifile& ini,
    FrontAPI& front,
    ClientInfo const& client_info,
    ClientExecute& rail_client_execute,
    KeyLayout const& layout,
    kbdtypes::KeyLocks locks,
    Ref<Font const> glyphs,
    Theme & theme,
    EventContainer& events,
    SessionLogApi& session_log,
    ErrorMessageCtx& err_msg_ctx,
    Random & rand);
