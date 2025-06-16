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
    Copyright (C) Wallix 2016
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#pragma once

#include "mod/rdp/channels/rail_channel.hpp"
#include "mod/rdp/channels/sespro_channel.hpp"
#include "mod/rdp/channels/sespro_launcher.hpp"

class SessionProbeAlternateShellBasedLauncher final : public SessionProbeLauncher {
private:
    RemoteProgramsVirtualChannel* rail_channel   = nullptr;
    SessionProbeVirtualChannel*   sespro_channel = nullptr;

    bool drive_redirection_initialized = false;

    bool image_readed = false;

    bool stopped = false;

    const RDPVerbose verbose;

public:
    explicit SessionProbeAlternateShellBasedLauncher(RDPVerbose verbose)
    : verbose(verbose)
    {}

    bool on_drive_access() override {
        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
            "SessionProbeAlternateShellBasedLauncher :=> on_drive_access");

        if (this->stopped) {
            return false;
        }

        if (this->sespro_channel) {
            this->sespro_channel->give_additional_launch_time();
        }

        return false;
    }

    bool on_device_announce_responded(bool bSucceeded) override {
        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
            "SessionProbeAlternateShellBasedLauncher :=> on_device_announce_responded, Succeeded=%s", (bSucceeded ? "Yes" : "No"));

        if (!this->stopped) {
            if (bSucceeded) {
                if (this->sespro_channel) {
                    this->sespro_channel->give_additional_launch_time();
                }
            }
            else {
                this->drive_redirection_initialized = false;

                if (this->sespro_channel) {
                    this->sespro_channel->abort_launch();
                }
            }
        }

        return false;
    }

    bool on_drive_redirection_initialize() override {
        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
            "SessionProbeAlternateShellBasedLauncher :=> on_drive_redirection_initialize");

        this->drive_redirection_initialized = true;

        return false;
    }

    bool on_image_read(uint64_t offset, uint32_t length) override {
        (void)offset;
        (void)length;

        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
            "SessionProbeAlternateShellBasedLauncher :=> on_image_read");

        if (this->stopped) {
            return false;
        }

        this->image_readed = true;

        if (this->sespro_channel) {
            this->sespro_channel->give_additional_launch_time();
        }

        return false;
    }

    void set_remote_programs_virtual_channel(RemoteProgramsVirtualChannel* channel) override {
        this->rail_channel = channel;
    }

    void set_session_probe_virtual_channel(SessionProbeVirtualChannel* channel) override {
        this->sespro_channel = channel;
    }

    LauchFailureInfo stop(bool bLaunchSuccessful) override {
        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
            "SessionProbeAlternateShellBasedLauncher :=> stop");

        this->stopped = true;

        if (bLaunchSuccessful) {
            if (this->rail_channel) {
                this->rail_channel->confirm_session_probe_launch();
            }
            return LauchFailureInfo{};
        }

        // err_msg are translated in sesman
        auto result
            = !this->drive_redirection_initialized ? LauchFailureInfo{
                ERR_SESSION_PROBE_ASBL_FSVC_UNAVAILABLE,
                "Session Probe is not launched. "
                "File System Virtual Channel is unavailable. "
                "Please allow the drive redirection in the Remote Desktop Services settings of the target."
                ""_zv
            }
            : !this->image_readed ? LauchFailureInfo{
                ERR_SESSION_PROBE_ASBL_MAYBE_SOMETHING_BLOCKS,
                "Session Probe is not launched. "
                "Maybe something blocks it on the target. "
                "Is the target running under Microsoft Server products? "
                "The Command Prompt should be published as the RemoteApp program and accept any command-line parameters. "
                "Please also check the temporary directory to ensure there is enough free space."
                ""_zv
            }
            : LauchFailureInfo{
                ERR_SESSION_PROBE_ASBL_UNKNOWN_REASON,
                "Session Probe launch has failed for unknown reason."_zv
            };

        LOG(LOG_ERR, "SessionProbeAlternateShellBasedLauncher :=> %s", result.err_msg);
        return result;
    }

    [[nodiscard]] bool is_keyboard_sequences_started() const override {
        return false;
    }

    [[nodiscard]] bool is_stopped() const override {
        return this->stopped;
    }

    [[nodiscard]] bool may_synthesize_user_input() const override
    {
        return false;
    }
};
