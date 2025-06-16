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
Copyright (C) Wallix 2010-2019
Author(s): Jonathan Poelen
*/

#include "red_channels/clipboard.hpp"

#include "red_emscripten/em_asm.hpp"
#include "red_emscripten/val.hpp"

#include "utils/log.hpp"
#include "utils/sugar/overload.hpp"

#include "core/callback.hpp"

#include "core/RDP/clipboard.hpp"
#include "core/RDP/clipboard/format_list_serialize.hpp"
#include "core/channel_list.hpp"


namespace redjs::channels::clipboard
{

namespace
{
    constexpr int first_last_flags
      = CHANNELS::CHANNEL_FLAG_LAST
      | CHANNELS::CHANNEL_FLAG_FIRST
    ;

    bytes_view quick_utf16_av(bytes_view av)
    {
        auto first = av.begin();
        auto count = av.size() / 2;

        while (count > 0)
        {
            auto it = first;
            auto step = count / 2;
            it += step * 2;
            if (it[0] | it[1])
            {
                first = it + 2;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }

        return av = av.first(first - av.begin());
    }

    namespace constants
    {
        namespace file_group_descriptor_w
        {
            // flags(4) + reserved1(32) + fileAttributes(4) + reserved2(16) + lastWriteTime(8)
            constexpr std::size_t useless_attributes_size = 64;
            // sizeHigh(4) + sizeLow(4)
            constexpr std::size_t filesize_attribute_size = 8;
            // filenameUtf16(520)
            constexpr std::size_t filename_attribute_size = 520;
            constexpr std::size_t file_packet_size
                = useless_attributes_size
                + filename_attribute_size
                + filesize_attribute_size;
        }
    }

    Cliprdr::IsLongFormat is_long_format(uint32_t general_flags)
    {
        return Cliprdr::IsLongFormat(bool(general_flags & RDPECLIP::CB_USE_LONG_FORMAT_NAMES));
    }

    Cliprdr::IsAscii is_ascii_format(uint32_t msg_flags)
    {
        return Cliprdr::IsAscii(bool(msg_flags & RDPECLIP::CB_ASCII_NAMES));
    }

    uint32_t get_uint32_le_or_throw(bytes_view data, char const* message)
    {
        InStream in(data);
        check_throw(in, 4, message, ERR_RDP_DATA_TRUNCATED);
        return in.in_uint32_le();
    }
}


ClipboardChannel::ClipboardChannel(Callback& cb, emscripten::val&& callbacks, bool verbose)
: cb(cb)
, callbacks(std::move(callbacks))
, verbose(verbose)
, channel_receiver(make_channel_receiver<&ClipboardChannel::receive>(
    channel_names::cliprdr, this))
{}

ClipboardChannel::~ClipboardChannel()
{
    emval_call(this->callbacks, "free");
}

void ClipboardChannel::send_file_contents_request(
    uint32_t request_type,
    uint32_t stream_id, uint32_t lindex,
    uint32_t pos_low, uint32_t pos_high,
    uint32_t max_bytes_to_read,
    bool has_lock_id, uint32_t lock_id)
{
    StaticOutStream<64> out_stream;

    RDPECLIP::FileContentsRequestPDU request(
        stream_id,
        lindex,
        request_type,
        pos_low,
        pos_high,
        (request_type == RDPECLIP::FILECONTENTS_SIZE)
            ? RDPECLIP::FILECONTENTS_SIZE_CB_REQUESTED
            : max_bytes_to_read,
        lock_id,
        has_lock_id && bool(this->general_flags & RDPECLIP::CB_CAN_LOCK_CLIPDATA)
    );

    RDPECLIP::CliprdrHeader header(
        RDPECLIP::CB_FILECONTENTS_REQUEST,
        RDPECLIP::CB_RESPONSE_NONE,
        request.size());

    header.emit(out_stream);
    request.emit(out_stream);
    this->send_data(out_stream.get_produced_bytes());
}

void ClipboardChannel::send_request_format(uint32_t format_id, CustomFormat custom_format_id)
{
    LOG_IF(this->verbose, LOG_INFO,
        "Clipboard: Send Request Format id=%u custom=%u", format_id, custom_format_id);

    RDPECLIP::CliprdrHeader formatListRequestPDUHeader(RDPECLIP::CB_FORMAT_DATA_REQUEST, RDPECLIP::CB_RESPONSE_NONE, 4);
    RDPECLIP::FormatDataRequestPDU formatDataRequestPDU(format_id);
    StaticOutStream<256> out_stream;
    formatListRequestPDUHeader.emit(out_stream);
    formatDataRequestPDU.emit(out_stream);
    InStream chunkRequest(out_stream.get_produced_bytes());

    this->requested_format_id = format_id;
    this->requested_custom_format_id = custom_format_id;

    this->send_data(out_stream.get_produced_bytes());
}

void ClipboardChannel::receive(bytes_view data, uint32_t total_data_len, uint32_t channel_flags)
{
    (void)total_data_len;

    switch (this->response_state)
    {
    case ResponseState::Data:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format Data Response PDU Continuation");
        this->process_format_data_response(data, channel_flags, this->remaining_data_len);
        return;

    case ResponseState::FileContents:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU Continuation");
        this->process_filecontents_response(data, channel_flags, this->remaining_data_len);
        return;

    case ResponseState::None:
        break;
    }

    InStream chunk(data);
    RDPECLIP::CliprdrHeader header;
    header.recv(chunk);

    if (header.msgFlags() == RDPECLIP::CB_RESPONSE_FAIL)
    {
        LOG(LOG_WARNING, "Clipboard: Response FAIL, msgType=%s",
            RDPECLIP::get_msgType_name(header.msgType()));
        // TODO always ?
        this->requested_format_id = 0;
        this->requested_custom_format_id = CustomFormat::None;
        emval_call(this->callbacks, "receiveResponseFail", header.msgType());
        return ;
    }

    switch (header.msgType())
    {
    case RDPECLIP::CB_CLIP_CAPS:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Capabilities PDU");
        this->process_capabilities(chunk);
        break;

    case RDPECLIP::CB_MONITOR_READY:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Monitor Ready PDU");
        this->process_monitor_ready();
        break;

    case RDPECLIP::CB_FORMAT_LIST_RESPONSE:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format List Response PDU");
        break;

    case RDPECLIP::CB_FORMAT_LIST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format List PDU");

        LOG_IF(header.dataLen() > chunk.in_remain(), LOG_WARNING,
            "Server Format List PDU data length(%u) longer than chunk(%zu)",
            header.dataLen(), chunk.in_remain());

        this->process_format_list(chunk, channel_flags, header.msgFlags());
        break;

    case RDPECLIP::CB_FORMAT_DATA_RESPONSE:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format Data Response PDU");
        this->process_format_data_response(chunk.remaining_bytes(), channel_flags, header.dataLen());
    break;

    case RDPECLIP::CB_FILECONTENTS_RESPONSE: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU");
        this->process_filecontents_response(chunk.remaining_bytes(), channel_flags, header.dataLen());
    }
    break;

    case RDPECLIP::CB_FORMAT_DATA_REQUEST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format Data Request PDU");
        this->process_format_data_request(chunk);
    break;

    case RDPECLIP::CB_FILECONTENTS_REQUEST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Request PDU");

        this->process_filecontents_request(chunk);
    break;

    case RDPECLIP::CB_LOCK_CLIPDATA:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Lock");
        emval_call(this->callbacks, "lock", get_uint32_le_or_throw(
            chunk.remaining_bytes(), "Clipboard::Lock"));
    break;

    case RDPECLIP::CB_UNLOCK_CLIPDATA:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Unlock");
        emval_call(this->callbacks, "unlock", get_uint32_le_or_throw(
            chunk.remaining_bytes(), "Clipboard::Unlock"));
    break;

    default:
        LOG_IF(this->verbose, LOG_ERR,
            "Clipboard: Default Process server PDU data (%" PRIu16 ")", header.msgType());
        // this->process_server_clipboard_indata(flags, chunk, this->_cb_buffers, this->_cb_filesList);
        break;
    }
}

void ClipboardChannel::process_format_data_request(InStream& in_stream)
{
    RDPECLIP::FormatDataRequestPDU pdu;
    pdu.recv(in_stream);
    emval_call(this->callbacks, "formatDataRequest", pdu.requestedFormatId);
}

void ClipboardChannel::process_filecontents_request(InStream& in_stream)
{
    RDPECLIP::FileContentsRequestPDU pdu;
    pdu.receive(in_stream);
    emval_call(this->callbacks, "fileContentsRequest",
        pdu.streamId(),
        pdu.dwFlags(),
        pdu.lindex(),
        pdu.position_low(),
        pdu.position_high(),
        pdu.cbRequested(),
        pdu.has_optional_clipDataId(),
        pdu.clipDataId());
}

namespace
{
    struct Utf8AsUtf16
    {
        bytes_view _utf8_bytes;
        std::size_t size() const noexcept { return _utf8_bytes.size(); }
        uint8_t const* data() const noexcept { return _utf8_bytes.data(); }
    };

    // This is a hack
    struct FormatNameUtf8AsUtf16
    {
        uint32_t _format_id;
        Utf8AsUtf16 _bytes;

        uint32_t format_id() const noexcept { return this->_format_id; }
        Utf8AsUtf16 utf8_name() const noexcept { return this->_bytes; }
    };

    std::size_t UTF8toUTF16(Utf8AsUtf16 const& utf16, writable_bytes_view target) noexcept
    {
        auto len = std::min(utf16._utf8_bytes.size(), target.size());
        len = len - (len & 1u);
        memcpy(target.as_charp(), utf16._utf8_bytes.data(), len);
        return len;
    }

    bool is_ASCII_string(Utf8AsUtf16 const&) noexcept
    {
        return false;
    }
}

unsigned ClipboardChannel::add_format(writable_bytes_view data, uint32_t format_id, Charset charset, bytes_view name)
{
    Cliprdr::FormatNameRef format{format_id, name};
    OutStream out_stream(data);

    switch (charset)
    {
        case Charset::Ascii:
            if (bool(is_long_format(this->general_flags)))
            {
                Cliprdr::format_list_serialize_long_format(out_stream, format);
            }
            else
            {
                Cliprdr::format_list_serialize_ascii_format(out_stream, format);
            }
            break;

        case Charset::Utf16:
            if (bool(is_long_format(this->general_flags)))
            {
                Cliprdr::format_list_serialize_long_format(out_stream,
                    FormatNameUtf8AsUtf16{format_id, {name}});
            }
            else
            {
                Cliprdr::format_list_serialize_unicode_format(out_stream, format);
            }
    }

    return out_stream.get_offset();
}

void ClipboardChannel::send_format(uint32_t format_id, Charset charset, bytes_view name)
{
    StaticOutStream<128> out_stream;

    switch (charset)
    {
        case Charset::Ascii:
            Cliprdr::format_list_serialize_with_header(
                out_stream,
                is_long_format(this->general_flags),
                std::array<FormatNameUtf8AsUtf16, 1>{{{format_id, {name}}}});
            break;

        case Charset::Utf16:
            Cliprdr::format_list_serialize_with_header(
                out_stream,
                is_long_format(this->general_flags),
                std::array<Cliprdr::FormatNameRef, 1>{{{format_id, name}}});
    }

    this->send_data(out_stream.get_produced_bytes());
}

void ClipboardChannel::send_header(uint16_t type, uint16_t flags, uint32_t total_data_len, uint32_t channel_flags)
{
    StaticOutStream<12> out_stream;

    RDPECLIP::CliprdrHeader header(type, flags, total_data_len);
    header.emit(out_stream);

    this->send_data(out_stream.get_produced_bytes(), total_data_len + out_stream.get_offset(),
        channel_flags | CHANNELS::CHANNEL_FLAG_FIRST);
}

void ClipboardChannel::send_data(bytes_view data, uint32_t total_data_len, uint32_t channel_flags)
{
    InStream in_stream(data);
    this->cb.send_to_mod_channel(
        channel_names::cliprdr,
        in_stream,
        total_data_len,
        channel_flags | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL
    );
}

void ClipboardChannel::send_data(bytes_view av)
{
    this->send_data(av, av.size(), first_last_flags);
}

void ClipboardChannel::process_format_data_response(
    bytes_view data, uint32_t channel_flags, uint32_t data_len)
{
    const bool is_first_packet = (channel_flags & CHANNELS::CHANNEL_FLAG_FIRST);
    const bool is_last_packet = (channel_flags & CHANNELS::CHANNEL_FLAG_LAST);

    if (is_first_packet)
    {
        this->response_buffer.clear();
        this->remaining_data_len = data_len;
    }

    this->response_state = is_last_packet
        ? ResponseState::None
        : ResponseState::Data;

    if (data.size() > this->remaining_data_len)
    {
        data = data.first(this->remaining_data_len);
    }
    this->remaining_data_len -= data.size();

    switch (this->requested_custom_format_id)
    {
    case CustomFormat::FileGroupDescriptorW: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Group Descriptor Response PDU");

        InStream in_stream(data);

        if (is_first_packet)
        {
            auto nb_item = in_stream.in_uint32_le();
            emval_call(this->callbacks, "formatDataResponseFileStart", nb_item);
        }

        namespace constants = constants::file_group_descriptor_w;

        auto extract_file = [&](InStream& in_stream){
            auto flags = in_stream.in_uint32_le();
            in_stream.in_skip_bytes(32);
            auto file_attrs = in_stream.in_uint32_le();
            in_stream.in_skip_bytes(16);
            auto last_write_time_high = in_stream.in_uint32_le();
            auto last_write_time_low = in_stream.in_uint32_le();
            auto size_high = in_stream.in_uint32_le();
            auto size_low = in_stream.in_uint32_le();
            auto name = quick_utf16_av(in_stream.in_skip_bytes(constants::filename_attribute_size));
            emval_call(this->callbacks, "formatDataResponseFile",
                name, file_attrs, flags, size_low, size_high,
                last_write_time_low, last_write_time_high);
        };

        if (this->response_buffer.size
            && in_stream.in_remain() + this->response_buffer.size >= constants::file_packet_size)
        {
            auto nbcopy = constants::file_packet_size - this->response_buffer.size;
            this->response_buffer.push(in_stream.in_skip_bytes(nbcopy));

            InStream in_stream(this->response_buffer.as_bytes());
            extract_file(in_stream);
            assert(in_stream.in_remain() == 0);

            this->response_buffer.clear();
        }

        while (in_stream.in_remain() >= constants::file_packet_size)
        {
            extract_file(in_stream);
        }

        this->response_buffer.push(in_stream.remaining_bytes());

        if (is_last_packet)
        {
            this->requested_format_id = 0;
            this->requested_custom_format_id = CustomFormat::None;
            emval_call(this->callbacks, "formatDataResponseFileStop");
        }

        break;
    }

    case CustomFormat::None:
        switch (this->requested_format_id)
        {
            case RDPECLIP::CF_UNICODETEXT:
                if (is_last_packet)
                {
                    const auto len = data.size();
                    if (len >= 2 && !data[len-1] && !data[len-2]) {
                        data = data.drop_back(2);
                    }
                }
                break;
        }

        emval_call(this->callbacks, "formatDataResponse",
            data, this->remaining_data_len, this->requested_format_id,
            channel_flags & first_last_flags);

        if (is_last_packet)
        {
            this->requested_format_id = 0;
        }
    }
}

void ClipboardChannel::process_filecontents_response(bytes_view data, uint32_t channel_flags, uint32_t data_len)
{
    const bool is_first_packet = (channel_flags & CHANNELS::CHANNEL_FLAG_FIRST);

    if (is_first_packet)
    {
        InStream in_stream(data);
        ::check_throw(in_stream, 4, "process_filecontents_response", ERR_RDP_DATA_TRUNCATED);
        this->stream_id = in_stream.in_uint32_le();
        this->remaining_data_len = data_len - 4u;
        data = in_stream.remaining_bytes();
    }

    this->response_state = bool(channel_flags & CHANNELS::CHANNEL_FLAG_LAST)
        ? ResponseState::None
        : ResponseState::FileContents;

    if (data.size() > this->remaining_data_len)
    {
        data = data.first(this->remaining_data_len);
    }
    this->remaining_data_len -= data.size();

    emval_call(this->callbacks, "fileContentsResponse",
        data, this->stream_id, this->remaining_data_len,
        channel_flags & first_last_flags);
}

void ClipboardChannel::process_format_list(InStream& chunk, uint32_t /*channel_flags*/, uint16_t msg_flags)
{
    emval_call(this->callbacks, "formatListStart");

    auto is_utf8 = overload{
        [](Cliprdr::AsciiName) { return true; },
        [](Cliprdr::UnicodeName) { return false; },
    };

    Cliprdr::format_list_extract(
        chunk,
        is_long_format(this->general_flags),
        is_ascii_format(msg_flags),
        [&](uint32_t format_id, auto const& name){
            CustomFormat custom_format_id = Cliprdr::file_group_descriptor_w.same_as(name)
                ? CustomFormat::FileGroupDescriptorW
                : CustomFormat::None;

            emval_call(this->callbacks, "formatListFormat",
                name.bytes, format_id, custom_format_id, is_utf8(name));
        }
    );

    emval_call(this->callbacks, "formatListStop");

    StaticOutStream<256> out_stream;
    RDPECLIP::CliprdrHeader formatListResponsePDUHeader(
        RDPECLIP::CB_FORMAT_LIST_RESPONSE, RDPECLIP::CB_RESPONSE_OK, 0);
    formatListResponsePDUHeader.emit(out_stream);
    this->send_data(out_stream.get_produced_bytes());
}

void ClipboardChannel::process_capabilities(InStream& chunk)
{
    uint32_t general_flags = RDPECLIP::extract_clipboard_general_flags_capability(
        chunk.remaining_bytes(), this->verbose);

    this->requested_format_id = 0;
    this->requested_custom_format_id = CustomFormat::None;
    this->general_flags = emval_call<uint32_t>(
        this->callbacks, "setGeneralCapability", general_flags);
}

void ClipboardChannel::process_monitor_ready()
{
    // send Client Clipboard Capabilities
    {
        uint16_t cCapabilitiesSets = 1;
        RDPECLIP::ClipboardCapabilitiesPDU clipboard_caps_pdu(cCapabilitiesSets);
        RDPECLIP::GeneralCapabilitySet general_cap_set(
            RDPECLIP::CB_CAPS_VERSION_2, this->general_flags);
        RDPECLIP::CliprdrHeader header(
            RDPECLIP::CB_CLIP_CAPS, RDPECLIP::CB_RESPONSE_NONE,
            clipboard_caps_pdu.size() + general_cap_set.size());

        StaticOutStream<128> out_stream;
        header.emit(out_stream);
        clipboard_caps_pdu.emit(out_stream);
        general_cap_set.emit(out_stream);

        this->send_data(out_stream.get_produced_bytes());

        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Send Capabilities PDU");
    }

    // send (Empty) Format List
    {
        RDPECLIP::CliprdrHeader header(
            RDPECLIP::CB_FORMAT_LIST,
            RDPECLIP::CB_ASCII_NAMES,
            0);

        StaticOutStream<128> out_stream;

        header.emit(out_stream);

        this->send_data(out_stream.get_produced_bytes());

        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Send Empty Format List PDU");
    }
}


void ClipboardChannel::ResponseBuffer::push(bytes_view av)
{
    assert(av.size() + this->size <= this->data.size());
    memcpy(this->data.data() + this->size, av.data(), av.size());
    this->size += av.size();
}

bytes_view ClipboardChannel::ResponseBuffer::as_bytes() const
{
    return {this->data.data(), this->size};
}

} // namespace redjs::channel::clipboard


#include "red_emscripten/bind.hpp"

EMSCRIPTEN_BINDINGS(channel_clipboard)
{
    namespace clipboard = redjs::channels::clipboard;

    redjs::class_<clipboard::ClipboardChannel>("ClipboardChannel")
        .constructor([](uintptr_t icb, emscripten::val&& callbacks, bool verbose) {
            auto* pcb = redjs::from_memory_offset<Callback*>(icb);
            return new clipboard::ClipboardChannel(*pcb, std::move(callbacks), verbose);
        })
        .function_ptr("getChannelReceiver", [](clipboard::ClipboardChannel& clip) {
            return redjs::to_memory_offset(clip.get_channel_receiver());
        })
        .function_ptr("sendRequestFormat", [](clipboard::ClipboardChannel& clip,
            uint32_t format_id, int custom_format_id)
        {
            clip.send_request_format(format_id, clipboard::CustomFormat(custom_format_id));
        })
        .function_ptr("sendFileContentsRequest", [](clipboard::ClipboardChannel& clip,
            uint32_t request_type, uint32_t stream_id, uint32_t lindex,
            uint32_t pos_low, uint32_t pos_high, uint32_t max_bytes_to_read,
            bool has_lock_id, uint32_t lock_id)
        {
            clip.send_file_contents_request(request_type,
                stream_id, lindex, pos_low, pos_high, max_bytes_to_read, has_lock_id, lock_id);
        })
        .function_ptr("sendHeader", [](clipboard::ClipboardChannel& clip,
            uint16_t type, uint16_t flags, uint32_t total_data_len, uint32_t channel_flags)
        {
            clip.send_header(type, flags, total_data_len, channel_flags);
        })
        .function_ptr("sendData", [](clipboard::ClipboardChannel& clip,
            std::string data, uint32_t total_data_len, uint32_t channel_flags)
        {
            clip.send_data(data, total_data_len, channel_flags);
        })
        .function_ptr("sendRawData", [](clipboard::ClipboardChannel& clip,
            std::ptrdiff_t idata, std::size_t idata_len,
            uint32_t total_data_len, uint32_t channel_flags)
        {
            auto* ptr = redjs::from_memory_offset<uint8_t const*>(idata);
            clip.send_data({ptr, idata_len}, total_data_len, channel_flags);
        })
        .function_ptr("sendDataWithHeader", [](clipboard::ClipboardChannel& clip,
            uint16_t type, std::ptrdiff_t idata, std::size_t idata_len)
        {
            auto* ptr = redjs::from_memory_offset<uint8_t const*>(idata);
            clip.send_header(type, 1/*Ok*/, idata_len, 0);
            clip.send_data({ptr, idata_len}, 0, 2/*last*/);
        })
        .function_ptr("addFormat", [](clipboard::ClipboardChannel& clip,
            std::ptrdiff_t idata, std::size_t idata_len,
            uint32_t format_id, int charset, std::string name)
        {
            auto* ptr = redjs::from_memory_offset<uint8_t*>(idata);
            return clip.add_format({ptr, idata_len}, format_id, clipboard::Charset(charset), name);
        })
        .function_ptr("sendFormat", [](clipboard::ClipboardChannel& clip,
            uint32_t format_id, int charset, std::string name)
        {
            clip.send_format(format_id, clipboard::Charset(charset), name);
        })
        // .function("receive", &clipboard::ClipboardChannel::receive)
    ;
}
