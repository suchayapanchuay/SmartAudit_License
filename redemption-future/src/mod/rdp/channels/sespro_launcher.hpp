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
    Copyright (C) Wallix 2015
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#pragma once

#include "core/error.hpp"
#include "utils/sugar/noncopyable.hpp"
#include "utils/sugar/zstring_view.hpp"

#include <cstdint>


class RemoteProgramsVirtualChannel;
class SessionProbeVirtualChannel;

class SessionProbeLauncher : noncopyable
{
public:
    virtual ~SessionProbeLauncher() = default;

    virtual bool on_drive_access() = 0;

    virtual bool on_drive_redirection_initialize() = 0;

    virtual bool on_device_announce_responded(bool bSucceeded) = 0;

    virtual bool on_image_read(uint64_t offset, uint32_t length) = 0;

    virtual void set_remote_programs_virtual_channel(
        RemoteProgramsVirtualChannel* channel) = 0;

    virtual void set_session_probe_virtual_channel(
        SessionProbeVirtualChannel* channel) = 0;

    struct [[nodiscard]] LauchFailureInfo
    {
        error_type err_id;
        zstring_view err_msg;
    };

    /// \return a default \c LauchFailureInfo when \p bLaunchSuccessful is true,
    /// otherwise returns the reason for the error
    virtual LauchFailureInfo stop(bool bLaunchSuccessful) = 0;

    [[nodiscard]] virtual bool is_keyboard_sequences_started() const = 0;

    [[nodiscard]] virtual bool is_stopped() const = 0;

    [[nodiscard]] virtual bool may_synthesize_user_input() const = 0;
};
