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
Copyright (C) Wallix 2010-2021
Author(s): Raphaël ZHOU
*/

#pragma once

#include "core/channel_names.hpp"

static inline CHANNELS::ChannelNameId get_effective_auth_channel_name(CHANNELS::ChannelNameId auth_channel)
{
    switch (auth_channel) {
        case CHANNELS::ChannelNameId():
        case CHANNELS::ChannelNameId("*"):
            return CHANNELS::ChannelNameId("wablnch");
        default:
            return auth_channel;
    }
}
