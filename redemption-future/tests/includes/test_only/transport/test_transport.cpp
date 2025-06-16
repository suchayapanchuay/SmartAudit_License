/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan

   Transport layer abstraction
*/

#include <exception>
#include <algorithm>

#include <cstring>

#include "transport/transport.hpp"
#include "utils/sugar/bytes_view.hpp"

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/transport/test_transport.hpp"


namespace test_transport
{
namespace
{
    struct RemainingError : std::exception
    {
        const char * what() const noexcept override
        {
            return "remaining data";
        }
    };

    struct hexdump
    {
        bytes_view sig;
    };

    struct hexdump_trailing
    {
        char const * type;
        uint8_t const * p;
        std::size_t len;

        static hexdump_trailing from_bytes(char const * type, bytes_view av)
        {
            return {type, av.data(), av.size()};
        }

        bytes_view sig() const
        {
            return {this->p, this->len};
        }
    };

    bool is_full_dump()
    {
        static bool x = []{
            char const* s = std::getenv("REDEMPTION_FULL_DUMP");
            return s && *s == '1';
        }();
        return x;
    }

    std::ostream & operator<<(std::ostream & out, hexdump const & data_dump)
    {
        auto x = data_dump.sig;
        if (!is_full_dump()) {
            x = x.first(std::min(data_dump.sig.size(), std::size_t(128)));
        }

        char buffer[2048];
        for (size_t j = 0 ; j < x.size(); j += 16){
            char * line = buffer;
            line += std::sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
            size_t i = 0;
            for (i = 0; i < 16; i++){
                if (j+i >= x.size()){ break; }
                line += std::sprintf(line, "\\x%.2x", static_cast<unsigned>(x.data()[j+i]));
            }
            line += std::sprintf(line, "\"");
            if (i < 16){
                line += std::sprintf(line, "%s", &
                    "                "
                    "                "
                    "                "
                    "                "
                    [i * 4u]);
            }
            line += std::sprintf(line, " // ");
            for (i = 0; i < 16; i++){
                if (j+i >= x.size()){ break; }
                unsigned char tmp = x.data()[j+i];
                if ((tmp < ' ') || (tmp > '~') || (tmp == '\\')){
                    tmp = '.';
                }
                line += std::sprintf(line, "%c", tmp);
            }

            if (line != buffer){
                line[0] = 0;
                out << buffer << "\n";
                buffer[0]=0;
            }
        }

        if (x.size() != data_dump.sig.size()) {
            out << "... followed by " << (data_dump.sig.size() - x.size()) << " bytes (set REDEMPTION_FULL_DUMP to 1 for a complete trace)\n";
        }

        return out;
    }

    std::ostream & operator<<(std::ostream & out, hexdump_trailing const & x)
    {
        return out
            << hexdump{x.sig()} << '\n'
            << "~" << x.type << "() remaining=" << x.sig().size()
        ;
    }
} // anonymous namespace
} // namespace test_transport

GeneratorTransport::GeneratorTransport(bytes_view buffer)
: impl(buffer)
{}

GeneratorTransport::~GeneratorTransport() noexcept(false)
{
    this->impl.disconnect();
}

GeneratorTransport::Impl::Impl(bytes_view buffer)
: data(new(std::nothrow) uint8_t[buffer.size()])
, len(buffer.size())
{
    if (!this->data) {
        throw Error(ERR_TRANSPORT_OPEN_FAILED);
    }
    if (data) {
        memcpy(this->data.get(), buffer.data(), this->len);
    }
}

bool GeneratorTransport::Impl::disconnect()
{
    if (this->remaining_is_error && this->len != this->current) {
        this->remaining_is_error = false;
        RED_CHECK_MESSAGE(this->len == this->current,
            "\n~GeneratorTransport() remaining=" << (this->len-this->current)
            << " len=" << this->len << "\n"
            << (test_transport::hexdump_trailing{
                "GeneratorTransport", this->data.get() + this->current, this->len - this->current
            })
        );
        if (!std::uncaught_exceptions()) {
            throw test_transport::RemainingError();
        }
    }
    return true;
}

Transport::Read GeneratorTransport::Impl::do_atomic_read(uint8_t * buffer, size_t len)
{
    size_t const remaining = this->len - this->current;
    if (!remaining) {
        return Read::Eof;
    }
    if (len > remaining) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }
    memcpy(buffer, this->data.get() + this->current, len);
    this->current += len;
    return Read::Ok;
}

size_t GeneratorTransport::Impl::do_partial_read(uint8_t* buffer, size_t len)
{
    size_t const remaining = this->len - this->current;
    if (!remaining) {
        return 0;
    }
    len = std::min(len, remaining);
    memcpy(buffer, this->data.get() + this->current, len);
    this->current += len;
    return len;
}


void BufTransport::do_send(const uint8_t * const data, size_t len)
{
    this->buf.append(char_ptr_cast(data), len);
}

Transport::Read BufTransport::do_atomic_read(uint8_t * buffer, size_t len)
{
    if (this->buf.empty()) {
        return Read::Eof;
    }

    if (this->buf.size() < len) {
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }

    memcpy(buffer, this->buf.data(), len);
    this->buf.erase(0, len);
    return Read::Ok;
}

size_t BufTransport::do_partial_read(uint8_t* buffer, size_t len)
{
    len = std::min(len, this->buf.size());
    memcpy(buffer, this->buf.data(), len);
    this->buf.erase(0, len);
    return len;
}


CheckTransport::CheckTransport(buffer_view buffer)
: impl(buffer)
{}

CheckTransport::~CheckTransport() noexcept(false)
{
    this->impl.disconnect();
}

CheckTransport::Impl::Impl(buffer_view buffer)
: data(new(std::nothrow) uint8_t[buffer.size()])
, len(buffer.size())
{
    if (!this->data) {
        throw Error(ERR_TRANSPORT, 0);
    }
    memcpy(this->data.get(), buffer.data(), this->len);
}

bool CheckTransport::Impl::disconnect()
{
    if (this->remaining_is_error && this->len != this->current) {
        this->remaining_is_error = false;
        RED_CHECK_MESSAGE(this->len == this->current,
            "\n~CheckTransport() reamining=0 failed, remaining=" << (this->len-this->current)
            << " len=" << this->len << "\n"
            << (test_transport::hexdump_trailing{
                "CheckTransport", this->data.get() + this->current, this->len - this->current
            })
        );
        if (!std::uncaught_exceptions()) {
            throw test_transport::RemainingError();
        }
    }
    return true;
}

void CheckTransport::Impl::do_send(const uint8_t * const data, size_t len)
{
    const size_t available_len = std::min<size_t>(this->len - this->current, len);

    if (0 != memcmp(data, this->data.get() + this->current, available_len)){
        // data differs, find where
        uint32_t differs = 0;
        while (differs < available_len && data[differs] == this->data.get()[this->current+differs]) {
            ++differs;
        }
        RED_TEST_INFO("current position: " << this->current);
        RED_CHECK(
            make_array_view(this->data.get() + this->current, available_len) ==
            make_array_view(data, len));
        this->data.reset();
        this->remaining_is_error = false;
        throw Error(ERR_TRANSPORT_DIFFERS);
    }

    // RED_REQUIRE(
    //     make_array_view(this->data.get() + this->current, available_len) ==
    //     make_array_view(data, len));

    this->current += len;

    // if (available_len != len || len == 0){
    //     RED_CHECK_MESSAGE(available_len == len,
    //         "check transport out of reference data available="
    //             << available_len << " len=" << len << " failed\n"
    //         "=============== Common Part =======\n" <<
    //         (test_transport::hexdump{{data, available_len}}) <<
    //         "=============== Got Unexpected Data =======\n" <<
    //         (test_transport::hexdump{{data + available_len, len - available_len}})
    //     );
    //     this->data.reset();
    //     this->remaining_is_error = false;
    //     throw Error(ERR_TRANSPORT_DIFFERS);
    // }
}


TestTransport::TestTransport(bytes_view indata, bytes_view outdata)
: impl(indata, outdata)
{}

TestTransport::~TestTransport() noexcept(false)
{
    impl.gen.disconnect();
    impl.check.disconnect();
}

TestTransport::Impl::Impl(bytes_view indata, bytes_view outdata)
: check(outdata)
, gen(indata)
{}

void TestTransport::set_public_key(bytes_view key)
{
    this->impl.public_key.reset(new uint8_t[key.size()]);
    this->impl.public_key_length = key.size();
    memcpy(this->impl.public_key.get(), key.data(), key.size());
}

u8_array_view TestTransport::Impl::get_public_key() const
{
    return {this->public_key.get(), this->public_key_length};
}

Transport::Read TestTransport::Impl::do_atomic_read(uint8_t * buffer, size_t len)
{
    return this->gen.atomic_read(buffer, len);
}

size_t TestTransport::Impl::do_partial_read(uint8_t* buffer, size_t len)
{
    return this->gen.partial_read(buffer, len);
}

void TestTransport::Impl::do_send(const uint8_t * const buffer, size_t len)
{
    this->check.send(buffer, len);
}


MemoryTransport::~MemoryTransport() noexcept(false)
{
    this->impl.disconnect();
}

bool MemoryTransport::Impl::disconnect()
{
    if (this->remaining_is_error && this->in_stream.get_offset() != this->out_stream.get_offset()) {
        RED_CHECK_MESSAGE(this->in_stream.get_offset() == this->out_stream.get_offset(),
            "\n~MemoryTransport() remaining=0 failed, remaining=" << this->in_stream.in_remain()
            << " len=" << this->out_stream.tailroom() << "\n"
            << (test_transport::hexdump_trailing::from_bytes(
                "MemoryTransport[in]", this->in_stream.remaining_bytes()
            )) << "\n"
            << (test_transport::hexdump_trailing::from_bytes(
                "MemoryTransport[out]", this->out_stream.get_tail()
            ))
        );
        if (!std::uncaught_exceptions()) {
            throw test_transport::RemainingError();
        }
    }
    return true;
}

Transport::Read MemoryTransport::Impl::do_atomic_read(uint8_t * buffer, size_t len)
{
    auto const in_offset = this->in_stream.get_offset();
    auto const out_offset = this->out_stream.get_offset();
    if (in_offset == out_offset){
        return Read::Eof;
    }
    if (in_offset + len > out_offset){
        throw Error(ERR_TRANSPORT_READ_FAILED);
    }
    this->in_stream.in_copy_bytes(buffer, len);
    return Read::Ok;
}

size_t MemoryTransport::Impl::do_partial_read(uint8_t* buffer, size_t len)
{
    auto const in_offset = this->in_stream.get_offset();
    auto const out_offset = this->out_stream.get_offset();
    if (in_offset == out_offset){
        return 0;
    }
    len = std::min(out_offset - out_offset, len);
    this->in_stream.in_copy_bytes(buffer, len);
    return len;
}

void MemoryTransport::Impl::do_send(const uint8_t * const buffer, size_t len)
{
    if (len > this->out_stream.tailroom()) {
        throw Error(ERR_TRANSPORT_WRITE_FAILED);
    }
    this->out_stream.out_copy_bytes(buffer, len);
}
