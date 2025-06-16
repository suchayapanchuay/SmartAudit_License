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
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan, Clément Moroldo
 */

#include "transport/mwrm_reader.hpp"
#include "utils/sugar/chars_to_int.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "utils/hexadecimal_string_to_buffer.hpp"
#include "utils/log.hpp"

#include <algorithm>

#include <cerrno>


// buffer must be able to contain line
// if no line at end of buffer apply some memmove
/// \exception Error : ERR_TRANSPORT_READ_FAILED
Transport::Read MwrmLineReader::next_line()
{
    if (this->eol > this->eof) {
        return Transport::Read::Eof;
    }
    this->cur = this->eol;
    char * pos = std::find(this->cur, this->eof, '\n');
    if (pos == this->eof) {
        // move remaining data to beginning of buffer
        size_t len = this->eof - this->cur;
        ::memmove(this->buf, this->cur, len);
        this->cur = this->buf;
        this->eof = this->cur + len;

        do { // read and append to buffer
            size_t ret = this->ibuf.partial_read(this->eof, std::end(this->buf)-2-this->eof);
            if (ret == 0) {
                break;
            }
            pos = std::find(this->eof, this->eof + ret, '\n');
            this->eof += ret;
            this->eof[0] = 0;
        } while (pos == this->eof);
    }

    this->eol = (pos == this->eof) ? pos : pos + 1;

    // end of file
    if (this->buf == this->eof) {
        return Transport::Read::Eof;
    }

    // line without \n is a error
    if (pos == this->eof) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    return Transport::Read::Ok;
}

namespace
{
    void init_stat_v1(MetaLine & meta_line)
    {
        meta_line.size = 0;
        meta_line.mode = 0;
        meta_line.uid = 0;
        meta_line.gid = 0;
        meta_line.dev = 0;
        meta_line.ino = 0;
        meta_line.mtime = 0;
        meta_line.ctime = 0;
    }

    char const * extract_hash(uint8_t (&hash)[MD_HASH::DIGEST_LENGTH], char const * in, int & err)
    {
        auto output = make_writable_array_view(hash);
        auto input = chars_view(in, output.size() * 2);
        err |= !hexadecimal_string_to_buffer(input, output);
        return in + input.size();
    }

    char * buf_sread_filename(char * p, char const * e, char * pline)
    {
        e -= 1;
        for (; p < e && *pline && *pline != ' ' && (*pline == '\\' ? *++pline : true); ++pline, ++p) {
            *p = *pline;
        }
        *p = 0;
        return pline;
    }

    // unchecked version of decimal_chars_to_int(s, value)
    template<class Int>
    Int consume_number(char const * & s){
        auto r = decimal_chars_to_int<Int>(s);
        s = r.ptr;
        return r.val;
    }
} // namespace

MwrmReader::MwrmReader(InTransport ibuf) noexcept
: line_reader(ibuf)
, header(WrmVersion::v1, false)
{
}

void MwrmReader::read_meta_headers()
{
    auto next_line = [this]{
        if (Transport::Read::Eof == this->line_reader.next_line()) {
            throw Error(ERR_TRANSPORT_READ_FAILED, errno);
        }
    };

    next_line();
    this->header = MetaHeader(WrmVersion::v1, false);
    if (this->line_reader.get_buf()[0] == 'v') {
        next_line(); // 800 600
        char const* wh_line = this->line_reader.get_buf().data();
        this->header.witdh = consume_number<uint16_t>(wh_line);
        if (*wh_line == ' ') {
            ++wh_line;
        }
        this->header.height = consume_number<uint16_t>(wh_line);
        next_line(); // checksum or nochecksum
        this->header.version = WrmVersion::v2;
        this->header.has_checksum = (this->line_reader.get_buf()[0] == 'c');
    }
    // else v1
    next_line(); // blank
    next_line(); // blank
}

void MwrmReader::set_header(MetaHeader const & header)
{
    this->header = header;
}

Transport::Read MwrmReader::read_meta_line(MetaLine & meta_line)
{
    switch (this->header.version) {
    case WrmVersion::v1:
        init_stat_v1(meta_line);
        return this->read_meta_line_v1(meta_line);
    case WrmVersion::v2:
        return this->read_meta_line_v2(meta_line, FileType::Mwrm);
    }
    assert(false);
    throw Error(ERR_TRANSPORT_READ_FAILED);
}

void MwrmReader::read_meta_hash_line(MetaLine & meta_line)
{
    Transport::Read r = Transport::Read::Eof;
    switch (this->header.version) {
    case WrmVersion::v1:
        init_stat_v1(meta_line);
        r = this->read_meta_hash_line_v1(meta_line);
        break;
    case WrmVersion::v2:
        bool is_eof = false;
        // skip header: "v2\n\n\n"
        is_eof |= Transport::Read::Eof == this->line_reader.next_line();
        is_eof |= Transport::Read::Eof == this->line_reader.next_line();
        is_eof |= Transport::Read::Eof == this->line_reader.next_line();
        if (is_eof) {
            throw Error(ERR_TRANSPORT_READ_FAILED);
        }
        r = this->read_meta_line_v2(meta_line, FileType::Hash);
        break;
    }

    if (r != Transport::Read::Ok || Transport::Read::Eof != this->line_reader.next_line()) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }
}

Transport::Read MwrmReader::read_meta_hash_line_v1(MetaLine & meta_line)
{
    // don't use line_reader.next_line() because '\n' is a valid hash character :/
    auto line_buf = [this]{
        auto * const buf = std::begin(this->line_reader.buf);
        auto * first = buf;
        auto * last = std::end(this->line_reader.buf);
        do { // read and append to buffer
            size_t ret = this->line_reader.ibuf.partial_read(first, last-first);
            if (ret == 0) {
                break;
            }
            first += ret;
        } while (first != last);
        return make_array_view(buf, first);
    }();

    meta_line.with_hash = true;

    // Filename HASH_64_BINARY_BYTES
    //         ^
    //         |
    //     separator

    constexpr std::size_t hash_size = 32u;
    constexpr std::size_t size_after_filename = hash_size * 2 + 1;

    static_assert(hash_size == sizeof(meta_line.hash1));
    static_assert(hash_size == sizeof(meta_line.hash2));

    if (line_buf.size() <= size_after_filename) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }
    if (line_buf.size() - size_after_filename >= sizeof(meta_line.filename)-1) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    auto const path_len = line_buf.size() - size_after_filename;
    memcpy(meta_line.filename, line_buf.begin(), path_len);
    meta_line.filename[path_len] = 0;
    memcpy(meta_line.hash1, line_buf.end() - 64, sizeof(meta_line.hash1));
    memcpy(meta_line.hash2, line_buf.end() - 32, sizeof(meta_line.hash1));

    return Transport::Read::Ok;
}

Transport::Read MwrmReader::read_meta_line_v1(MetaLine & meta_line)
{
    if (Transport::Read::Eof == this->line_reader.next_line()) {
        return Transport::Read::Eof;
    }

    // Line format "fffff sssss eeeee hhhhh HHHHH"
    //                               ^  ^  ^  ^
    //                               |  |  |  |
    //                               |hash1|  |
    //                               |     |  |
    //                           space3    |hash2
    //                                     |
    //                                   space4
    //
    // filename(1 or >) + space(1) + start_sec(1 or >) + space(1) + stop_sec(1 or >) +
    //     space(1) + hash1(64) + space(1) + hash2(64) >= 135

    // TODO Code below looks much too complicated for what it's doing

    using reverse_iterator = std::reverse_iterator<char*>;

    using std::begin;

    auto line_buf = this->line_reader.get_buf();

    reverse_iterator last(line_buf.begin());
    reverse_iterator first(line_buf.end());
    reverse_iterator e1 = std::find(first, last, ' ');
    int err = 0;

    meta_line.with_hash = (e1 - first == 65);
    if (meta_line.with_hash) {
        extract_hash(meta_line.hash2, e1.base(), err);
    }

    reverse_iterator e2 = (e1 == last) ? e1 : std::find(e1 + 1, last, ' ');
    if (e2 - e1 == 65) {
        extract_hash(meta_line.hash1, e2.base(), err);
    }
    else if (meta_line.with_hash) {
        LOG(LOG_ERR, "mwrm read line v1: fhash without qhash");
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    if (err) {
        LOG(LOG_ERR, "mwrm read line v1: invalid hash");
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    bool const has_hash = (e1 - first == 65 && e2 != last);

    if (has_hash) {
        first = e2 + 1;
        e1 = std::find(first, last, ' ');
        e2 = (e1 == last) ? e1 : std::find(e1 + 1, last, ' ');
    }

    chars_to_int_result<long long> r;

    r = decimal_chars_to_int<long long>(e1.base());
    meta_line.stop_time = std::chrono::seconds(r.val);
    err |= (*r.ptr != (has_hash ? ' ' : '\n'));
    if (e1 != last) {
        ++e1;
    }

    r = decimal_chars_to_int<long long>(e2.base());
    meta_line.start_time = std::chrono::seconds(r.val);
    err |= (*r.ptr != ' ');
    if (e2 != last) {
        *e2 = 0;
    }

    if (err) {
        LOG(LOG_ERR, "mwrm read line v1: invalid format");
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    auto const path_len = std::min(size_t(e2.base() - line_buf.begin()), sizeof(meta_line.filename)-1);
    memcpy(meta_line.filename, line_buf.begin(), path_len);
    meta_line.filename[path_len] = 0;

    return Transport::Read::Ok;
}

Transport::Read MwrmReader::read_meta_line_v2(MetaLine & meta_line, FileType file_type)
{
    if (Transport::Read::Eof == this->line_reader.next_line()) {
        return Transport::Read::Eof;
    }

    // Line format "fffff
    // st_size st_mode st_uid st_gid st_dev st_ino st_mtime st_ctime
    // sssss eeeee hhhhh HHHHH"
    //            ^  ^  ^  ^
    //            |  |  |  |
    //            |hash1|  |
    //            |     |  |
    //        space3    |hash2
    //                  |
    //                space4
    //
    // filename(1 or >) + space(1) + stat_info(ll|ull * 8) +
    //     space(1) + start_sec(1 or >) + space(1) + stop_sec(1 or >) +
    //     space(1) + hash1(64) + space(1) + hash2(64) >= 135
    //
    // sssss and eeeee only with FileType::Mwrm
    // hhhhh and HHHHH only with has_checksum

    auto line_buf = this->line_reader.get_buf();
    auto const line = line_buf.data();

    using std::begin;
    using std::end;

    char const * pline = buf_sread_filename(begin(meta_line.filename), end(meta_line.filename), line);

    int err = 0;

    auto pre_consume = [&]{
        if (*pline == ' ') {
            ++pline;
        } else {
            err = 1;
        }
    };

    auto consume_ll = [&]{
        pre_consume();
        return consume_number<long long>(pline);
    };

    auto consume_ull = [&]{
        pre_consume();
        return consume_number<unsigned long long>(pline);
    };

    meta_line.size  = consume_ll();
    meta_line.mode  = checked_int(consume_ull());
    meta_line.uid   = checked_int(consume_ll());
    meta_line.gid   = checked_int(consume_ll());
    meta_line.dev   = consume_ull();
    meta_line.ino   = checked_int(consume_ll());
    meta_line.mtime = consume_ll();
    meta_line.ctime = consume_ll();

    if (file_type == FileType::Mwrm) {
        meta_line.start_time = std::chrono::seconds(consume_ll());
        meta_line.stop_time  = std::chrono::seconds(consume_ll());
    }
    else {
        meta_line.start_time = std::chrono::seconds();
        meta_line.stop_time  = std::chrono::seconds();
    }

    meta_line.with_hash = (file_type == FileType::Mwrm)
      ? this->header.has_checksum
      : (this->header.has_checksum || (*pline != '\n' && *pline != '\0'));
    if (meta_line.with_hash) {
        // ' ' hash ' ' hash '\n'
        err |= line_buf.size() - (pline - line) != (sizeof(meta_line.hash1) + sizeof(meta_line.hash2)) * 2 + 3;
        if (!err)
        {
            err |= (*pline != ' '); pline = extract_hash(meta_line.hash1, ++pline, err);
            err |= (*pline != ' '); pline = extract_hash(meta_line.hash2, ++pline, err);
        }
    }
    err |= (*pline != '\n' && *pline != '\0');

    if (err) {
        LOG(LOG_ERR, "mwrm read line v2: invalid format");
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    return Transport::Read::Ok;
}


void MwrmWriterBuf::write_header(MetaHeader const & header) noexcept
{
    this->write_header(header.witdh, header.height, header.has_checksum);
}

void MwrmWriterBuf::write_header(uint16_t width, uint16_t height, bool has_checksum) noexcept
{
    this->len = sprintf(this->mes, "v2\n%u %u\n%s\n\n\n",
        unsigned(width),  unsigned(height), has_checksum ? "checksum" : "nochecksum");
}

struct PrivateMwrmWriterBuf
{
    MwrmWriterBuf & writer;
    using HashArray = MwrmWriterBuf::HashArray;

    template<class Stat>
    void mwrm_write_line(
        char const * filename, Stat const & stat,
        bool with_time, std::chrono::seconds start_time, std::chrono::seconds stop_time,
        bool with_hash, MwrmWriterBuf::HashArray const & qhash, MwrmWriterBuf::HashArray const & fhash
    ) noexcept
    {
        assert(writer.len < MwrmWriterBuf::max_header_size);

        write_filename(filename);
        write_stat(stat);
        if (with_time) {
            write_start_and_stop(start_time, stop_time);
        }
        if (with_hash) {
            write_hashs(qhash, fhash);
        }
        write_newline();
    }

    void write_hash_header() noexcept
    {
        writer.len = sprintf(writer.mes, "v2\n\n\n");
    }

private:
    void write_filename(char const * filename) noexcept
    {
        for (size_t i = 0; (filename[i]) && (writer.len < PATH_MAX-2) ; i++){
            switch (filename[i]){
            case '\\':
            case ' ':
                writer.mes[writer.len++] = '\\';
                [[fallthrough]];
            default:
                writer.mes[writer.len++] = filename[i];
            break;
            }
        }
    }

    void write_stat(struct stat const & stat) noexcept
    {
        using ull = unsigned long long;
        using ll = long long;
        writer.len += std::sprintf(
            writer.mes + writer.len,
            " %lld %llu %lld %lld %llu %lld %lld %lld",
            ll(stat.st_size),
            ull(stat.st_mode),
            ll(stat.st_uid),
            ll(stat.st_gid),
            ull(stat.st_dev),
            ll(stat.st_ino),
            ll(stat.st_mtim.tv_sec),
            ll(stat.st_ctim.tv_sec)
        );
    }

    void write_stat(MetaLine const & meta_line) noexcept
    {
        using ull = unsigned long long;
        using ll = long long;
        writer.len += std::sprintf(
            writer.mes + writer.len,
            " %lld %llu %lld %lld %llu %lld %lld %lld",
            ll(meta_line.size),
            ull(meta_line.mode),
            ll(meta_line.uid),
            ll(meta_line.gid),
            ull(meta_line.dev),
            ll(meta_line.ino),
            ll(meta_line.mtime),
            ll(meta_line.ctime)
        );
    }

    void write_start_and_stop(std::chrono::seconds start, std::chrono::seconds stop) noexcept
    {
        using ll = long long;
        writer.len += std::sprintf(
            writer.mes + writer.len,
            " %lld %lld",
            ll(start.count()),
            ll(stop.count())
        );
    }

    void write_hashs(HashArray const & qhash, HashArray const & fhash) noexcept
    {
        char * p = writer.mes + writer.len;

        auto hexdump = [&p](uint8_t const (&hash)[MD_HASH::DIGEST_LENGTH]) {
            *p++ = ' '; // 1 octet
            // 64 octets (hash)
            for (uint8_t c : hash) {
                p = int_to_fixed_hexadecimal_lower_chars(p, c);
            }
        };
        hexdump(qhash);
        hexdump(fhash);

        writer.len = p - writer.mes;
    }

    void write_newline() noexcept
    {
        writer.mes[writer.len++] = '\n';
        writer.mes[writer.len] = 0;
    }
};

void MwrmWriterBuf::write_line(MetaLine const & meta_line) noexcept
{
    this->reset_buf();
    PrivateMwrmWriterBuf{*this}.mwrm_write_line(
        meta_line.filename, meta_line,
        true, meta_line.start_time, meta_line.stop_time,
        meta_line.with_hash, meta_line.hash1, meta_line.hash2);
}

void MwrmWriterBuf::write_line(
    char const * filename, struct stat const & stat,
    std::chrono::seconds start_time, std::chrono::seconds stop_time,
    bool with_hash, HashArray const & qhash, HashArray const & fhash) noexcept
{
    this->reset_buf();
    PrivateMwrmWriterBuf{*this}.mwrm_write_line(
        filename, stat,
        true, start_time, stop_time,
        with_hash, qhash, fhash);
}

void MwrmWriterBuf::write_hash_file(MetaLine const & meta_line) noexcept
{
    PrivateMwrmWriterBuf{*this}.write_hash_header();
    PrivateMwrmWriterBuf{*this}.mwrm_write_line(
        meta_line.filename, meta_line,
        false, std::chrono::seconds(), std::chrono::seconds(),
        meta_line.with_hash, meta_line.hash1, meta_line.hash2);
}

void MwrmWriterBuf::write_hash_file(
    char const * filename, struct stat const & stat,
    bool with_hash, HashArray const & qhash, HashArray const & fhash) noexcept
{
    PrivateMwrmWriterBuf{*this}.write_hash_header();
    PrivateMwrmWriterBuf{*this}.mwrm_write_line(
        filename, stat,
        false, std::chrono::seconds(), std::chrono::seconds(),
        with_hash, qhash, fhash);
}

chars_view MwrmWriterBuf::buffer() const noexcept
{
    return {this->mes, this->len};
}

char const * MwrmWriterBuf::c_str() const noexcept
{
    return this->mes;
}

bool MwrmWriterBuf::is_full() const noexcept
{
    return sizeof(this->mes)-1 >= this->len;
}
