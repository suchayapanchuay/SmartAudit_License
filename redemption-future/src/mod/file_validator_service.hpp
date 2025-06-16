/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2019
*   Author(s): Clément Moroldo
*/

#pragma once

#include "core/buf64k.hpp"
#include "core/error.hpp"

#include "transport/transport.hpp"

#include "utils/stream.hpp"
#include "utils/sugar/cast.hpp"
#include "utils/sugar/numerics/safe_conversions.hpp"
#include "utils/sugar/int_to_chars.hpp"

#include <array>
#include <string>
#include <numeric>
#include <string_view>
#include <cinttypes>

#include "utils/log.hpp"


enum class FileValidatorId : uint32_t;

namespace LocalFileValidatorProtocol
{

inline void emit_file_id(OutStream& stream, FileValidatorId file_id) noexcept
{
    stream.out_uint32_be(safe_int(file_id));
}

inline FileValidatorId recv_file_id(InStream& stream) noexcept
{
    return safe_int(stream.in_uint32_be());
}

enum class MsgType : uint8_t
{
    // Send data from a file
    DataFile = 0x01,
    // Close the session
    CloseSession = 0x02,
    // Indicates data file end
    Eof = 0x03,
    // Abort file.
    AbortFile = 0x04,
    // Result received from Validator
    Result = 0x05,
    // map with key(string), value(string)
    Infos = 0x06,
    // Create a new data to analyze
    NewData = 0x07,

    Unknown,
};

enum class ValidationResult : uint8_t
{
    IsAccepted,
    IsRejected,
    Error,
    Wait,
};

enum class [[nodiscard]] ReceiveStatus : bool
{
    WaitingData,
    Ok,
};

struct FileValidatorHeader
{
    // This header starts every message receive from or emit to the local service.

    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | | | | | | | | | | |1| | | | | | | | | |2| |
    // |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |   msg_type    |          msg_size         |
    // +---------------+----------------------------

    MsgType msg_type;
    uint32_t msg_len;


    FileValidatorHeader() = default;

    FileValidatorHeader(MsgType msg_type, uint32_t msg_len) noexcept
    : msg_type(msg_type)
    , msg_len(msg_len)
    {}

    void emit(OutStream& stream) const noexcept
    {
        stream.out_uint8(safe_int(this->msg_type));
        stream.out_uint32_be(this->msg_len);
    }

    ReceiveStatus recv(InStream& stream) noexcept
    {
        if (stream.in_remain() < 5) {
            return ReceiveStatus::WaitingData;
        }

        this->msg_type = safe_int(stream.in_uint8());
        this->msg_len = stream.in_uint32_be();

        return ReceiveStatus::Ok;
    }
};


struct FileValidatorResultHeader
{
    ValidationResult result = ValidationResult::Wait;
    FileValidatorId file_id;
    uint32_t content_size;

    // ResultMessage

    // This message is send from the local service to sessions, it does NOT
    // need an FileValidatorHeader so.

    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
    // |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |    Result     |                  file_id                      |
    // +---------------+-----------------------------------------------+
    // |               |               content_length                  |
    // +---------------+-----------------------------------------------+
    // |               |                  content                      |
    // +---------------+-----------------------------------------------+
    // |                             ...                               |
    // +---------------------------------------------------------------+

    // Result: An unsigned, 8-bit integer that indicate result analysis. Value
    //         must be ACCEPTED_FLAG, REJECTED_FLAG or Error_FLAG.

    //   +--------------------+----------------------------------------------+
    //   | Value              | Meaning                                      |
    //   +--------------------+----------------------------------------------+
    //   | ACCEPTED_FLAG      | File is valid                                |
    //   | 0x00               |                                              |
    //   +--------------------+----------------------------------------------+
    //   | REJECTED_FLAG      | File is NOT valid                            |
    //   | 0x01               |                                              |
    //   +--------------------+----------------------------------------------+
    //   | Error_FLAG         | File analysis error                          |
    //   | 0x02               |                                              |
    //   +--------------------+----------------------------------------------+

    // file_id: An unsigned, 32-bit integer that contains client file id sent
    //          to the validator.

    // content_length: An unsigned, 32-bit integer that contains result content
    //                 length.

    // content: A variable length, ascii string that contains analysis result.

    ReceiveStatus recv(InStream& stream) noexcept
    {
        /* Result(1) + File_id_size(4) + content_size(4)  */
        if (stream.in_remain() < 9) {
            return ReceiveStatus::WaitingData;
        }

        this->result = safe_int(stream.in_uint8());
        this->file_id = safe_int(stream.in_uint32_be());
        this->content_size = stream.in_uint32_be();

        return ReceiveStatus::Ok;
    }

    void emit(OutStream& stream) const noexcept
    {
        stream.out_uint8(safe_int(this->result));
        stream.out_uint32_be(safe_int(this->file_id));
        stream.out_uint32_be(safe_int(this->content_size));
    }
};

inline std::size_t open_data_map_len(array_view<bytes_view> data_map)
{
    assert(0 == (data_map.size() & 1));

    const std::size_t data_len = std::accumulate(data_map.begin(), data_map.end(), std::size_t(),
        [](auto n, auto const av) { return n + av.size(); });
    return 2u + data_len + data_map.size() * 2u;
}

inline void send_data_map(
    OutTransport trans, OutStream& message, array_view<bytes_view> data_map)
{
    message.out_uint16_be(data_map.size() / 2);

    for (auto const& data : data_map) {
        if (!message.has_room(data.size() + 2u)) {
            trans.send(message.get_produced_bytes());
            message.rewind();
        }
        assert(message.has_room(data.size() + 2u));
        message.out_uint16_be(checked_int(data.size()));
        message.out_copy_bytes(data);
    }

    trans.send(message.get_produced_bytes());
}

inline FileValidatorId send_open_data(
    OutTransport trans, FileValidatorId file_id, std::string_view target_name,
    array_view<bytes_view> data_map)
{
    /*
    NewData

    This message is sent to create a new data to request icap analyse
    for. It begins with an ICAPHeader, its msg_type must be NewData.

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
    |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                            file_id                            |
    +---------------------------------------------------------------+
    |        target_name_size       |       target_name_data        |
    +---------------------------------------------------------------+
    |           nb_items            |       item1_key_length        |
    +---------------------------------------------------------------+
    |                    item1_key      ...                         |
    +---------------------------------------------------------------+
    |      item1_value_length       |       item1_value ...         |
    +---------------------------------------------------------------+
    |       item2_key_length        |        item2_key ...          |
    +---------------------------------------------------------------+
    |      item2_value_length       |       item2_value ...         |
    +---------------------------------------------------------------+
    |                              ...                              |
    +---------------------------------------------------------------+

    file_id: 32-bits integer

    target_name_size: 16-bits integer
    target_name_data: String of size target_name_size

    nb_items: An unsigned, 16-bits integer that the number of items.

    item<i>_key_length: An unsigned, 16-bits integer size of the <i>th
                         item key length.
    item<i>_key: String of size item<i>_key_length
    item<i>_value_length: An unsigned, 16-bits integer size of the <i>th
                           item value length.
    item<i>_value: String of size item<i>_value_length
    */

    const std::size_t pkt_len = 4u + 2u + target_name.size() + open_data_map_len(data_map);

    StaticOutStream<8*1024> message;

    FileValidatorHeader(MsgType::NewData, pkt_len).emit(message);

    emit_file_id(message, file_id);

    message.out_uint16_be(checked_int(target_name.size()));
    message.out_copy_bytes(target_name);

    send_data_map(trans, message, data_map);

    return file_id;
}

inline void send_header(
    OutTransport trans, FileValidatorId file_id, MsgType msg_type, uint32_t pkt_len)
{
    StaticOutStream<16> message;

    FileValidatorHeader(msg_type, 4u + pkt_len)
    .emit(message);

    emit_file_id(message, file_id);

    trans.send(message.get_produced_bytes());
}

inline void send_data_file(OutTransport trans, FileValidatorId file_id, bytes_view data)
{
    constexpr std::ptrdiff_t max_pkt_len = 0x0fff'ffff;

    auto p = data.begin();
    const auto pend = data.end();

    while (p != pend) {
        const auto pkt_len = std::min(pend-p, max_pkt_len);
        send_header(trans, file_id, MsgType::DataFile, pkt_len);
        trans.send({p, std::size_t(pkt_len)});
        p += pkt_len;
    }
}

/// data_map is a key value list
inline void send_infos(OutTransport trans, std::initializer_list<bytes_view> data_map)
{
    /*
    InfosMessage

    This message contains additionnal infos for the session.
    It begins with an FileValidatorHeader its msg_type must be INFOS_FLAG.

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
    |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |           nb_items            |       item1_key_length        |
    +---------------------------------------------------------------+
    |                    item1_key      ...                         |
    +---------------------------------------------------------------+
    |      item1_value_length       |       item1_value ...         |
    +---------------------------------------------------------------+
    |       item2_key_length        |        item2_key ...          |
    +---------------------------------------------------------------+
    |      item2_value_length       |       item2_value ...         |
    +---------------------------------------------------------------+
    |                              ...                              |
    +---------------------------------------------------------------+

    nb_items : An unsigned, 16-bits integer that the number of items.

    item<i>_key_length : An unsigned, 16-bits integer size of the <i>th
                         item key length.
    item<i>_key : String of size item<i>_key_length
    item<i>_value_length : An unsigned, 16-bits integer size of the <i>th
                           item value length.
    item<i>_value : String of size item<i>_value_length
    */

    const std::size_t pkt_len = open_data_map_len(data_map);

    StaticOutStream<8*1024> message;

    FileValidatorHeader(MsgType::Infos, pkt_len).emit(message);
    send_data_map(trans, message, data_map);
}

} // namespace LocalFileValidatorProtocol

struct FileValidatorService
{
    enum class Verbose : bool
    {
        No,
        Yes
    };

    FileValidatorService(Transport& trans, Verbose verbose) noexcept
    : trans(trans)
    , verbose(bool(verbose))
    {}

    FileValidatorId open_file(std::string_view filename, std::string_view target_name)
    {
        auto file_id = this->open_raw_data(target_name, data_map_array("filename"_av, filename));
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::open_file(): id=%u", file_id);
        return file_id;
    }

    FileValidatorId open_text(uint32_t locale_identifier, std::string_view target_name)
    {
        auto chars = int_to_decimal_chars(locale_identifier);
        auto file_id = this->open_raw_data(target_name,
            data_map_array("microsoft_locale_id"_av, chars));
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::open_text(): id=%u", file_id);
        return file_id;
    }

    FileValidatorId open_unicode(std::string_view target_name)
    {
        auto file_id = this->open_raw_data(target_name, data_map_array("format_id"_av, "13"_av));
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::open_unicode(): id=%u", file_id);
        return file_id;
    }

    void send_data(FileValidatorId file_id, bytes_view data)
    {
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::send_data(id=%u, data.len=%zu)", file_id, data.size());
        LocalFileValidatorProtocol::send_data_file(this->trans, file_id, data);
    }

    /// data_map is a key value list
    void send_infos(std::initializer_list<bytes_view> data_map)
    {
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::send_infos()");
        LocalFileValidatorProtocol::send_infos(this->trans, data_map);
    }

    void send_eof(FileValidatorId file_id)
    {
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::send_eof(id=%u)", file_id);
        using namespace LocalFileValidatorProtocol;
        send_header(this->trans, file_id, MsgType::Eof, 0);
    }

    void send_abort(FileValidatorId file_id)
    {
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::send_abort(id=%u)", file_id);
        using namespace LocalFileValidatorProtocol;
        send_header(this->trans, file_id, MsgType::AbortFile, 0);
    }

    void send_close_session()
    {
        LOG_IF(verbose, LOG_INFO, "FileValidatorService::send_close_session()");
        using namespace LocalFileValidatorProtocol;
        send_header(this->trans, FileValidatorId(), MsgType::CloseSession, 0);
    }

    [[nodiscard]] std::string const& get_content() const noexcept
    {
        return this->content;
    }

    [[nodiscard]] FileValidatorId last_file_id() const noexcept
    {
        return this->result_header.file_id;
    }

    [[nodiscard]] LocalFileValidatorProtocol::ValidationResult last_result_flag() const noexcept
    {
        return this->result_header.result;
    }

    enum class [[nodiscard]] ResponseType : uint8_t
    {
        WaitingData,
        Error,
        HasContent,
    };

    ResponseType receive_response()
    {
        if (this->state <= State::WaitingData) {
            this->buf.read_from(this->trans);
            // hexdump(this->buf.av());
        }

        this->state = this->_receive_response();
        return this->state == State::ContinuationData
            ? ResponseType::WaitingData
            : ResponseType(underlying_cast(this->state)-1);
        // switch (this->state) {
        //     case State::Error:      return ResponseType::Error;
        //     case State::HasContent: return ResponseType::HasContent;
        //     default:                return ResponseType::WaitingData;
        // }
    }

private:
    using DataMap = array_view<bytes_view>;

    template<class... Ts>
    static std::array<bytes_view, sizeof...(Ts)> data_map_array(Ts&&... xs)
    {
        return std::array{bytes_view(xs)...};
    }

    FileValidatorId open_raw_data(std::string_view target_name, DataMap data_map)
    {
        return LocalFileValidatorProtocol::send_open_data(
            this->trans, this->generate_id(), target_name, data_map);
    }

    enum class [[nodiscard]] State : uint8_t
    {
        ContinuationData,
        WaitingData,

        Error,
        HasContent,
    };

    State _receive_response()
    {
        InStream in_stream(this->buf.av());

        auto shift_data = [&](State r){
            this->buf.advance(in_stream.get_offset());
            return r;
        };

        auto read_content = [&]{
            std::size_t remaining_buf = this->result_header.content_size - this->content.size();
            std::size_t remaining_message = std::min(in_stream.in_remain(), remaining_buf);
            auto message = in_stream.in_skip_bytes(remaining_message);
            this->content.append(char_ptr_cast(message.data()), message.size());

            if (this->content.size() < this->result_header.content_size) {
                return shift_data(State::ContinuationData);
            }

            return shift_data(State::HasContent);
        };

        using namespace LocalFileValidatorProtocol;

        if (this->state == State::ContinuationData) {
            return read_content();
        }

        FileValidatorHeader header;
        if (header.recv(in_stream) != ReceiveStatus::Ok) {
            return State::WaitingData;
        }

        if (header.msg_type == MsgType::Result) {
            if (this->result_header.recv(in_stream) != ReceiveStatus::Ok) {
                return State::WaitingData;
            }

            this->content.clear();
            return read_content();
        }

        LOG(LOG_ERR, "FileValidatorService::receive_response: Unknown packet: msg_type=%d",
            int(header.msg_type));
        in_stream.in_skip_bytes(std::min<std::size_t>(header.msg_len, in_stream.in_remain()));
        return shift_data(State::Error);
    }

    FileValidatorId generate_id() noexcept
    {
        ++this->current_id;
        return FileValidatorId(this->current_id);
    }

    Transport& trans;
    LocalFileValidatorProtocol::FileValidatorResultHeader result_header;
    std::string content;
    int32_t current_id = 0;
    State state = State::WaitingData;
    bool verbose;

    BasicStaticBuffer<1024*16, uint16_t> buf;
};
