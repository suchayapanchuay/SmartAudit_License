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

#include "mod/rdp/channels/sespro_clipboard_based_launcher.hpp"

#include "mod/rdp/channels/sespro_channel.hpp"
#include "core/RDP/clipboard.hpp"
#include "core/RDP/clipboard/format_list_serialize.hpp"

using namespace std::chrono_literals;

namespace
{

using Ms = std::chrono::milliseconds;
using kbdtypes::KbdFlags;
using kbdtypes::Scancode;

static long long ms2ll(Ms ms)
{
    return ms.count();
}

static Ms to_microseconds(Ms delay, float coefficient)
{
    return Ms(static_cast<Ms::rep>(delay.count() * coefficient));
}

} // anonymous namespace

SessionProbeClipboardBasedLauncher::SessionProbeClipboardBasedLauncher(
    EventContainer& events,
    RdpSessionProbeWrapper rdp,
    const char* alternate_shell,
    Params params,
    RDPVerbose verbose)
: params(params)
, rdp(rdp)
, alternate_shell(alternate_shell)
, events_guard(events)
, verbose(verbose)
{
    this->params.clipboard_initialization_delay_ms = std::max(this->params.clipboard_initialization_delay_ms, 2000ms);
    this->params.long_delay_ms                     = std::max(this->params.long_delay_ms, 500ms);
    this->params.short_delay_ms                    = std::max(this->params.short_delay_ms, 50ms);

    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher: "
            "clipboard_initialization_delay_ms=%lld "
            "start_delay_ms=%lld "
            "long_delay_ms=%lld "
            "short_delay_ms=%lld",
        ms2ll(this->params.clipboard_initialization_delay_ms),
        ms2ll(this->params.start_delay_ms),
        ms2ll(this->params.long_delay_ms),
        ms2ll(this->params.short_delay_ms));
}

bool SessionProbeClipboardBasedLauncher::on_client_format_list_rejected()
{
    return this->restore_client_clipboard();
}

bool SessionProbeClipboardBasedLauncher::on_clipboard_initialize()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_clipboard_initialize");

    if (this->state != State::START) {
        return false;
    }

    this->clipboard_initialized = true;

    if (this->sesprob_channel) {
        this->sesprob_channel->give_additional_launch_time();
    }

    this->do_state_start();

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_clipboard_monitor_ready()
{
    this->clipboard_monitor_ready = true;

    if (this->state == State::START) {
        this->event_ref = this->events_guard.create_event_timeout(
            "SessionProbeClipboardBasedLauncher::on_clipboard_monitor_ready",
            this->params.clipboard_initialization_delay_ms,
            [this](Event&event)
            {
                this->on_event();
                event.garbage=true;
            });
    }

    if (this->sesprob_channel) {
        this->sesprob_channel->give_additional_launch_time();
    }

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_drive_access()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_drive_access");

    if (this->state != State::START) {
        return false;
    }

    if (this->sesprob_channel) {
        this->sesprob_channel->give_additional_launch_time();
    }

    this->drive_ready = true;

    this->do_state_start();

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_device_announce_responded(bool bSucceeded)
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_device_announce_responded, Succeeded=%s", (bSucceeded ? "Yes" : "No"));

    if (this->state == State::START) {
        this->drive_ready = bSucceeded;

        if (bSucceeded) {
            if (this->sesprob_channel) {
                this->sesprob_channel->give_additional_launch_time();
            }

            this->do_state_start();
        }
        else {
            this->drive_redirection_initialized = false;

            if (this->sesprob_channel) {
                this->sesprob_channel->abort_launch();
            }
        }
    }

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_drive_redirection_initialize()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_drive_redirection_initialize");

    this->drive_redirection_initialized = true;

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_event()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_event - %d", int(this->state));

    if (this->state == State::START) {
        if (CurClientClipboardInitStep::Invalid ==
            this->cur_client_clipboard_init_step)
        {
            this->clipboard_initialized_by_proxy = true;

            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                "SessionProbeClipboardBasedLauncher :=> launcher managed cliprdr initialization");

            // Client Clipboard Capabilities PDU.
            {
                RDPECLIP::GeneralCapabilitySet general_cap_set(
                    RDPECLIP::CB_CAPS_VERSION_1,
                    RDPECLIP::CB_USE_LONG_FORMAT_NAMES);

                RDPECLIP::ClipboardCapabilitiesPDU clipboard_caps_pdu(1);

                RDPECLIP::CliprdrHeader clipboard_header(RDPECLIP::CB_CLIP_CAPS, RDPECLIP::CB_RESPONSE_NONE,
                    clipboard_caps_pdu.size() + general_cap_set.size());

                StaticOutStream<1024> out_s;

                clipboard_header.emit(out_s);
                clipboard_caps_pdu.emit(out_s);
                general_cap_set.emit(out_s);

                this->get_next_filter_ptr()->process_client_message(
                        out_s.get_offset(),
                          CHANNELS::CHANNEL_FLAG_FIRST
                        | CHANNELS::CHANNEL_FLAG_LAST
                        | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                        out_s.get_produced_bytes(),
                        &clipboard_header
                    );
            }

            this->client_supports_long_format_name = true;

            this->cur_client_clipboard_init_step =
                CurClientClipboardInitStep::ClipboardCapabilities;
        }

        if (CurClientClipboardInitStep::ClipboardCapabilities ==
            this->cur_client_clipboard_init_step)
        {
            // Format List PDU.
            {
                StaticOutStream<256> out_s;
                Cliprdr::format_list_serialize_with_header(
                    out_s,
                    Cliprdr::IsLongFormat(this->use_long_format_name()),
                    std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

                this->get_next_filter_ptr()->process_client_message(
                        out_s.get_offset(),
                          CHANNELS::CHANNEL_FLAG_FIRST
                        | CHANNELS::CHANNEL_FLAG_LAST
                        | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                        out_s.get_produced_bytes(),
                        nullptr
                    );
            }
        }
    }
    else {
        LOG(LOG_WARNING, "SessionProbeClipboardBasedLauncher::on_event: Unexpected State=%d", this->state);
        assert(false);
    }

    return (this->state >= State::WAIT);
}   // bool on_event()

bool SessionProbeClipboardBasedLauncher::on_image_read(uint64_t offset, uint32_t length)
{
    (void)offset;
    (void)length;

    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_image_read");

    this->image_readed = true;

    if (this->state == State::STOP) {
        return false;
    }

    if (this->sesprob_channel) {
        this->sesprob_channel->give_additional_launch_time();
    }

    return true;
}

bool SessionProbeClipboardBasedLauncher::on_server_format_data_request()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_server_format_data_request");

    StaticOutStream<1024> out_s;
    size_t alternate_shell_length = this->alternate_shell.length() + 1;
    RDPECLIP::CliprdrHeader header(RDPECLIP::CB_FORMAT_DATA_RESPONSE, RDPECLIP::CB_RESPONSE_OK, alternate_shell_length);
    const RDPECLIP::FormatDataResponsePDU format_data_response_pdu;
    header.emit(out_s);
    format_data_response_pdu.emit(out_s, byte_ptr_cast(this->alternate_shell.c_str()), alternate_shell_length);

    this->get_next_filter_ptr()->process_client_message(
            out_s.get_offset(),
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
            out_s.get_produced_bytes(),
            &header
        );

    this->server_format_data_requested = true;

    if (this->sesprob_channel) {
        this->sesprob_channel->give_additional_launch_time();
    }

    return false;
}

bool SessionProbeClipboardBasedLauncher::on_server_format_list()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_server_format_list");

    this->delay_format_list_received = true;

    return false;
}

MonotonicTimePoint SessionProbeClipboardBasedLauncher::get_short_delay_timeout() const
{
    std::chrono::milliseconds delay_ms = this->params.short_delay_ms;
    return this->events_guard.get_monotonic_time()
            + std::min(to_microseconds(delay_ms, this->delay_coefficient), 1000ms);
}

MonotonicTimePoint SessionProbeClipboardBasedLauncher::get_long_delay_timeout() const
{
    std::chrono::milliseconds delay_ms = this->params.long_delay_ms;
    return this->events_guard.get_monotonic_time() + to_microseconds(delay_ms, this->delay_coefficient);
}

void SessionProbeClipboardBasedLauncher::make_delay_sequencer()
{
    LOG(LOG_INFO, "SessionProbeClipboardBasedLauncher make_delay_sequencer()");

    #define mk_seq(f) \
        f(Windows_down) \
        f(r_down) \
        f(r_up) \
        f(Windows_up) \
        f(Ctrl_down) \
        f(c_down) \
        f(c_up) \
        f(Ctrl_up) \
        f(Wait) \
        f(Send_format_list) \
        f(Wait_format_list_response)
    #define enum_ident(ident) ident,
    #define name_ident(ident) #ident,
    enum SeqEnum : uint8_t { mk_seq(enum_ident) };
    static char const* names[]{ mk_seq(name_ident) };
    #undef name_ident
    #undef enum_ident
    #undef mk_seq

    auto action = [this, i = Windows_down](Event& event) mutable {
        auto set_state = [&](SeqEnum newi){
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                    "SessionProbeClipboardBasedLauncher Event transition: %s => %s",
                    names[i], names[newi]);
            i = newi;
        };

        auto next_state = [&](KbdFlags flags, Scancode scancode, MonotonicTimePoint timeout){
            this->rdp.send_scancode(flags, scancode);
            event.set_timeout(timeout);
            set_state(SeqEnum(i+1));
        };

        for (;;) {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                    "SessionProbeClipboardBasedLauncher Event: %s", names[i]);

            switch (i) {

            case Windows_down:
                if (this->delay_format_list_received) {
                    i = Send_format_list;
                    continue;
                }
                return next_state(
                    KbdFlags::Extended, Scancode::LWin,
                    this->get_short_delay_timeout());

            case r_down:
                return next_state(
                    KbdFlags::NoFlags, Scancode::R,
                    this->get_short_delay_timeout());

            case r_up:
                return next_state(
                    KbdFlags::Release, Scancode::R,
                    this->get_short_delay_timeout());

            case Windows_up:
                return next_state(
                    KbdFlags::Extended | KbdFlags::Release, Scancode::LWin,
                    this->get_long_delay_timeout());

            case Ctrl_down:
                if (this->delay_format_list_received) {
                    i = Send_format_list;
                    continue;
                }
                return next_state(
                    KbdFlags::NoFlags, Scancode::LCtrl,
                    this->get_short_delay_timeout());

            case c_down:
                return next_state(
                    KbdFlags::NoFlags, Scancode::C,
                    this->get_short_delay_timeout());

            case c_up:
                return next_state(
                    KbdFlags::Release, Scancode::C,
                    this->get_short_delay_timeout());

            case Ctrl_up:
                return next_state(
                    KbdFlags::Release, Scancode::LCtrl,
                    this->get_long_delay_timeout());

            case Wait:
                if (events_guard.get_monotonic_time_since_epoch() < this->delay_end_time){
                    this->delay_coefficient += 0.5f;
                    set_state(Windows_down);
                }
                else {
                    set_state(Send_format_list);
                }
                continue;

            case Send_format_list: {
                this->delay_wainting_clipboard_response = true;
                StaticOutStream<256> out_s;
                Cliprdr::format_list_serialize_with_header(
                    out_s,
                    Cliprdr::IsLongFormat(this->use_long_format_name()),
                    std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

                this->get_next_filter_ptr()->process_client_message(
                        out_s.get_offset(),
                          CHANNELS::CHANNEL_FLAG_FIRST
                        | CHANNELS::CHANNEL_FLAG_LAST
                        | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                        out_s.get_produced_bytes(),
                        nullptr
                    );

                event.set_timeout(this->get_long_delay_timeout());
                set_state(Wait_format_list_response);
                event.garbage = true;
                return ;
            }

            case Wait_format_list_response:
                event.garbage = true;
                return ;

            }

            assert(false);
        }
    };

    this->event_ref = this->events_guard.create_event_timeout(
        "SessionProbeClipboardBasedLauncher Event",
        this->params.short_delay_ms,
        action);
}

void SessionProbeClipboardBasedLauncher::make_run_sequencer()
{
    #define mk_seq(f) \
        f(Windows_down) \
        f(r_down) \
        f(r_up) \
        f(Windows_up) \
        f(Ctrl_down) \
        f(v_down) \
        f(v_up) \
        f(Ctrl_up) \
        f(Enter_down) \
        f(Enter_up) \
        f(Wait)
    #define enum_ident(ident) ident,
    #define name_ident(ident) #ident,
    enum SeqEnum : uint8_t { mk_seq(enum_ident) };
    static char const* names[]{ mk_seq(name_ident) };
    #undef name_ident
    #undef enum_ident
    #undef mk_seq

    auto action = [this, i = Windows_down](Event& event) mutable {
        auto set_state = [&](SeqEnum newi){
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                    "SessionProbeClipboardBasedLauncher Event transition: %s => %s",
                    names[i], names[newi]);
            i = newi;
        };

        auto next_state = [&](KbdFlags flags, Scancode scancode, MonotonicTimePoint timeout){
            this->rdp.send_scancode(flags, scancode);
            event.set_timeout(timeout);
            set_state(SeqEnum(i+1));
        };

        for (;;) {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                    "SessionProbeClipboardBasedLauncher Event: %s", names[i]);

            switch (i) {

            case Windows_down:
                if (this->image_readed) {
                    event.garbage = true;
                    return;
                }
                return next_state(
                    KbdFlags::Extended, Scancode::LWin,
                    this->get_short_delay_timeout());

            case r_down:
                return next_state(
                    KbdFlags::NoFlags, Scancode::R,
                    this->get_short_delay_timeout());

            case r_up:
                return next_state(
                    KbdFlags::Release, Scancode::R,
                    this->get_short_delay_timeout());

            case Windows_up:
                return next_state(
                    KbdFlags::Extended | KbdFlags::Release, Scancode::LWin,
                    this->get_long_delay_timeout());

            case Ctrl_down:
                if (this->image_readed) {
                    event.garbage = true;
                    return;
                }
                return next_state(
                    KbdFlags::NoFlags, Scancode::LCtrl,
                    this->get_short_delay_timeout());

            case v_down:
                return next_state(
                    KbdFlags::NoFlags, Scancode::V,
                    this->get_short_delay_timeout());

            case v_up:
                return next_state(
                    KbdFlags::Release, Scancode::V,
                    this->get_short_delay_timeout());

            case Ctrl_up:
                return next_state(
                    KbdFlags::Release, Scancode::LCtrl,
                    this->get_long_delay_timeout());

            case Enter_down:
                ++this->copy_paste_loop_counter;
                if (!this->server_format_data_requested) {
                    // Back to the beginning of the sequence
                    set_state(Windows_down);
                    continue;
                }
                if (this->image_readed) {
                    event.garbage = true;
                    return;
                }
                return next_state(
                    KbdFlags::NoFlags, Scancode::Enter,
                    this->get_short_delay_timeout());

            case Enter_up:
                return next_state(
                    KbdFlags::Release, Scancode::Enter,
                    this->get_long_delay_timeout());

            case Wait:
                if (this->image_readed) {
                    this->state = State::WAIT;
                    event.garbage = true;
                    return;
                }
                else {
                    this->delay_coefficient += 0.5f;
                    set_state(Windows_down);
                    continue;
                }
            }

            assert(false);
        }
    };

    this->event_ref = this->events_guard.create_event_timeout(
        "SessionProbeClipboardBasedLauncher Event",
        this->params.short_delay_ms,
        action);
}

bool SessionProbeClipboardBasedLauncher::on_server_format_list_response()
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> on_server_format_list_response");

    if (this->params.start_delay_ms.count()) {
        if (!this->delay_executed) {
            if (this->state != State::START) {
                return (this->state < State::WAIT);
            }

            this->state = State::DELAY;

            make_delay_sequencer();

            auto const now = events_guard.get_monotonic_time_since_epoch();
            this->delay_end_time = now + this->params.start_delay_ms;

            this->delay_executed = true;
        }
        else if (this->delay_wainting_clipboard_response) {
            this->delay_wainting_clipboard_response = false;

            this->state = State::RUN;

            make_run_sequencer();

            this->delay_coefficient = 1.0f;

            return false;
        }

        return true;
    }

    if (this->state != State::START) {
        return (this->state < State::WAIT);
    }

    this->state = State::RUN;

    make_run_sequencer();

    return false;
}

void SessionProbeClipboardBasedLauncher::process_client_message(
    uint32_t total_length, uint32_t flags, bytes_view chunk_data,
    RDPECLIP::CliprdrHeader const* decoded_header)
{
    InStream stream(chunk_data);
    if (this->process_client_cliprdr_message(stream, total_length, flags))
    {
        this->get_next_filter_ptr()->process_client_message(total_length,
            flags, chunk_data, decoded_header);
    }
}

// Returns false to prevent message to be sent to server.
bool SessionProbeClipboardBasedLauncher::process_client_cliprdr_message(InStream & chunk, uint32_t length, uint32_t flags)
{
    (void)length;

    if (this->state == State::STOP) {
        return true;
    }

    bool ret = true;

    const size_t saved_chunk_offset = chunk.get_offset();

    if (((flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) == (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) &&
        (chunk.in_remain() >= 8 /* msgType(2) + msgFlags(2) + dataLen(4) */)) {
        const uint8_t* current_chunk_pos  = chunk.get_current();
        const size_t   current_chunk_size = chunk.in_remain();

        assert(current_chunk_size == length);

        const uint16_t msgType = chunk.in_uint16_le();
        if (RDPECLIP::CB_CLIP_CAPS == msgType) {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                "SessionProbeClipboardBasedLauncher :=> process_client_cliprdr_message(CB_CLIP_CAPS)");

            this->cur_client_clipboard_init_step =
                CurClientClipboardInitStep::ClipboardCapabilities;

            chunk.in_skip_bytes(6 /* msgFlags(2) + dataLen(4) */);

            RDPECLIP::ClipboardCapabilitiesPDU pdu;
            pdu.recv(chunk);

            RDPECLIP::GeneralCapabilitySet caps;
            caps.recv(chunk);

            this->client_supports_long_format_name =
                bool(caps.generalFlags() & RDPECLIP::CB_USE_LONG_FORMAT_NAMES);
        }
        else if (RDPECLIP::CB_FORMAT_LIST == msgType) {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                "SessionProbeClipboardBasedLauncher :=> process_client_cliprdr_message(CB_FORMAT_LIST)");

            this->cur_client_clipboard_init_step =
                CurClientClipboardInitStep::FormatList;

            this->current_client_format_list_pdu_length = current_chunk_size;
            this->current_client_format_list_pdu        =
                std::make_unique<uint8_t[]>(current_chunk_size);
            ::memcpy(this->current_client_format_list_pdu.get(),
                current_chunk_pos, current_chunk_size);
            this->current_client_format_list_pdu_flags  = flags;

            if (this->client_format_list_sent)
            {
                RDPECLIP::CliprdrHeader clipboard_header(RDPECLIP::CB_FORMAT_LIST_RESPONSE, RDPECLIP::CB_RESPONSE_OK, 0);

                StaticOutStream<128> out_s;

                clipboard_header.emit(out_s);

                const size_t totalLength = out_s.get_offset();
                this->get_previous_filter_ptr()->process_server_message(
                        totalLength,
                        CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST,
                        out_s.get_produced_bytes(),
                        &clipboard_header
                    );
            }
            else
            {
                StaticOutStream<256> out_s;
                Cliprdr::format_list_serialize_with_header(
                    out_s,
                    Cliprdr::IsLongFormat(this->use_long_format_name()),
                    std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

                const size_t totalLength = out_s.get_offset();

                this->get_next_filter_ptr()->process_client_message(
                        totalLength,
                            CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST
                        | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                        out_s.get_produced_bytes(),
                        nullptr
                    );

                this->client_format_list_sent = true;
            }

            ret = false;
        }
    }

    chunk.rewind(saved_chunk_offset);

    return ret;
}

void SessionProbeClipboardBasedLauncher::process_server_message(
    uint32_t total_length, uint32_t flags, bytes_view chunk_data,
    RDPECLIP::CliprdrHeader const* decoded_header)
{
    InStream stream(chunk_data);
    if ((this->process_server_cliprdr_message(stream, total_length, flags)) &&
        !this->clipboard_initialized_by_proxy)
    {
        this->get_previous_filter_ptr()->process_server_message(total_length,
            flags, chunk_data, decoded_header);
    }
}

// Returns false to prevent message to be sent to client.
bool SessionProbeClipboardBasedLauncher::process_server_cliprdr_message(InStream & chunk, REDEMPTION_UNUSED_IN_DEBUG uint32_t length, uint32_t flags) {
    if (((flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) == (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)) &&
        (chunk.in_remain() >= 8 /* msgType(2) + msgFlags(2) + dataLen(4) */)) {
        assert(chunk.in_remain() == length);

        const uint16_t msgType = chunk.in_uint16_le();
        if (this->state != State::STOP) {
            if (RDPECLIP::CB_CLIP_CAPS == msgType) {
                LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                    "SessionProbeClipboardBasedLauncher :=> process_server_cliprdr_message(CB_CLIP_CAPS)");

                chunk.in_skip_bytes(6 /* msgFlags(2) + dataLen(4) */);

                RDPECLIP::ClipboardCapabilitiesPDU pdu;
                pdu.recv(chunk);

                RDPECLIP::GeneralCapabilitySet caps;
                caps.recv(chunk);

                this->server_supports_long_format_name =
                    bool(caps.generalFlags() & RDPECLIP::CB_USE_LONG_FORMAT_NAMES);
            }
            else if (RDPECLIP::CB_FORMAT_DATA_REQUEST == msgType) {
                if (!this->server_format_data_requested)
                {
                    this->on_server_format_data_request();

                    return false;
                }
            }
            else if (RDPECLIP::CB_FORMAT_LIST == msgType) {
                if (!this->delay_format_list_received)
                {
                    this->on_server_format_list();
                }
            }
            else if (RDPECLIP::CB_FORMAT_LIST_RESPONSE == msgType) {
                if (!this->clipboard_initialized)
                {
                    this->on_clipboard_initialize();
                }

                if (!this->server_format_list_response_processed)
                {
                    this->server_format_list_response_processed = !this->on_server_format_list_response();
                }
            }
            else if (RDPECLIP::CB_MONITOR_READY == msgType) {
                this->on_clipboard_monitor_ready();
            }
        }
        else
        {
            if (RDPECLIP::CB_FORMAT_LIST_RESPONSE == msgType) {
                if (this->is_stopped()) {
                    const uint16_t msgFlags = chunk.in_uint16_le();
                    if (msgFlags == RDPECLIP::CB_RESPONSE_FAIL &&
                        this->format_list_rejection_retry_count < SessionProbeClipboardBasedLauncher::FORMAT_LIST_REJECTION_RETRY_MAX) {
                        if (!this->on_client_format_list_rejected()) {
                            this->format_list_rejection_retry_count = SessionProbeClipboardBasedLauncher::FORMAT_LIST_REJECTION_RETRY_MAX;

                            return false;
                        }

                        this->format_list_rejection_retry_count++;
                    }
                    else {
                        LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                            "SessionProbeClipboardBasedLauncher :=> Remove self.");
                        this->remove_self();
                    }
                }
            }
        }
    }

    return true;
}

void SessionProbeClipboardBasedLauncher::set_remote_programs_virtual_channel(RemoteProgramsVirtualChannel* /*channel*/)
{}

void SessionProbeClipboardBasedLauncher::set_session_probe_virtual_channel(SessionProbeVirtualChannel* channel) {
    this->sesprob_channel = channel;
}

SessionProbeLauncher::LauchFailureInfo SessionProbeClipboardBasedLauncher::stop(bool bLaunchSuccessful)
{
    LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
        "SessionProbeClipboardBasedLauncher :=> stop");

    this->state = State::STOP;
    this->event_ref.garbage();

    LauchFailureInfo result {};

    if (!bLaunchSuccessful) {
        // err_msg are translated in sesman
        result
            = !this->drive_redirection_initialized ? LauchFailureInfo{
                ERR_SESSION_PROBE_CBBL_FSVC_UNAVAILABLE,
                "Session Probe is not launched. "
                "File System Virtual Channel is unavailable. "
                "Please allow the drive redirection in the Remote Desktop Services settings of the target."
                ""_zv
            }
            : !this->clipboard_initialized ? LauchFailureInfo {
                ERR_SESSION_PROBE_CBBL_CBVC_UNAVAILABLE,
                "Session Probe is not launched. "
                "Clipboard Virtual Channel is unavailable. "
                "Please allow the clipboard redirection in the Remote Desktop Services settings of the target."
                ""_zv
            }
            : !this->drive_ready ? LauchFailureInfo{
                ERR_SESSION_PROBE_CBBL_DRIVE_NOT_READY_YET,
                "Session Probe is not launched. "
                "Drive of Session Probe is not ready yet. "
                "Is the target running under Windows Server 2008 R2 or more recent version?"
                ""_zv
            }
            : !this->image_readed ? LauchFailureInfo{
                ERR_SESSION_PROBE_CBBL_MAYBE_SOMETHING_BLOCKS,
                "Session Probe is not launched. "
                "Maybe something blocks it on the target. "
                "Please also check the temporary directory to ensure there is enough free space."
                ""_zv
            }
            : !this->copy_paste_loop_counter ? LauchFailureInfo{
                ERR_SESSION_PROBE_CBBL_LAUNCH_CYCLE_INTERRUPTED,
                "Session Probe launch cycle has been interrupted. "
                "The launch timeout duration may be too short."
                ""_zv
            }
            : LauchFailureInfo{
                ERR_SESSION_PROBE_CBBL_UNKNOWN_REASON_REFER_TO_SYSLOG,
                this->clipboard_monitor_ready && this->server_format_data_requested
                ? "Session Probe launch has failed for unknown reason. "
                  "clipboard_monitor_ready=yes server_format_data_requested=yes"_zv
                : !this->clipboard_monitor_ready && this->server_format_data_requested
                ? "Session Probe launch has failed for unknown reason. "
                  "clipboard_monitor_ready=no server_format_data_requested=yes"_zv
                : this->clipboard_monitor_ready && !this->server_format_data_requested
                ? "Session Probe launch has failed for unknown reason. "
                  "clipboard_monitor_ready=yes server_format_data_requested=no"_zv
                : "Session Probe launch has failed for unknown reason. "
                  "clipboard_monitor_ready=no server_format_data_requested=no"_zv
            };
        LOG(LOG_ERR, "SessionProbeClipboardBasedLauncher :=> %s", result.err_msg);
    }

    this->restore_client_clipboard();

    if (this->params.reset_keyboard_status) {
        this->rdp.reset_keyboard_status();
    }

    return result;
}

bool SessionProbeClipboardBasedLauncher::restore_client_clipboard()
{
    if (this->clipboard_initialized) {
        if (!this->clipboard_initialized_by_proxy && bool(this->current_client_format_list_pdu)) {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                "SessionProbeClipboardBasedLauncher :=> restore_client_clipboard");

            // Sends client Format List PDU to server
            this->get_next_filter_ptr()->process_client_message(
                    this->current_client_format_list_pdu_length,
                    this->current_client_format_list_pdu_flags,
                    {this->current_client_format_list_pdu.get(),
                     this->current_client_format_list_pdu_length},
                    nullptr
                );
        }
        else {
            LOG_IF(bool(this->verbose & RDPVerbose::sesprobe_launcher), LOG_INFO,
                "SessionProbeClipboardBasedLauncher :=> empty_client_clipboard");

            RDPECLIP::CliprdrHeader clipboard_header(RDPECLIP::CB_FORMAT_LIST,
                RDPECLIP::CB_RESPONSE_NONE, 0);

            StaticOutStream<256> out_s;

            clipboard_header.emit(out_s);

            this->get_next_filter_ptr()->process_client_message(
                    out_s.get_offset(),
                      CHANNELS::CHANNEL_FLAG_FIRST
                    | CHANNELS::CHANNEL_FLAG_LAST
                    | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                    out_s.get_produced_bytes(),
                    &clipboard_header
                );
        }

        return true;
    }

    return false;
}

void SessionProbeClipboardBasedLauncher::do_state_start()
{
    assert(State::START == this->state);

    if (!this->drive_ready || !this->clipboard_initialized) {
        return;
    }

    if (!this->client_format_list_sent)
    {
        StaticOutStream<256> out_s;
        Cliprdr::format_list_serialize_with_header(
            out_s,
            Cliprdr::IsLongFormat(this->use_long_format_name()),
            std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

        const size_t totalLength = out_s.get_offset();
        this->get_next_filter_ptr()->process_client_message(
                totalLength,
                  CHANNELS::CHANNEL_FLAG_FIRST
                | CHANNELS::CHANNEL_FLAG_LAST
                | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
                out_s.get_produced_bytes(),
                nullptr
            );
    }
}

[[nodiscard]] bool SessionProbeClipboardBasedLauncher::is_keyboard_sequences_started() const
{
    return (State::START != this->state);
}

[[nodiscard]] bool SessionProbeClipboardBasedLauncher::is_stopped() const
{
    return (this->state == State::STOP);
}

[[nodiscard]] bool SessionProbeClipboardBasedLauncher::may_synthesize_user_input() const
{
    return true;
}

[[nodiscard]] bool SessionProbeClipboardBasedLauncher::use_long_format_name() const
{
     return this->client_supports_long_format_name
         && this->server_supports_long_format_name;
}
