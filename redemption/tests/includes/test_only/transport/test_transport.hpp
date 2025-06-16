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

#pragma once

#include <vector>
#include <string>
#include <memory>

#include "transport/transport.hpp"
#include "utils/stream.hpp"
#include "utils/sugar/bytes_view.hpp"


template<class ImplTransport>
struct AutoDisconnectTransportWrapper
{
    ~AutoDisconnectTransportWrapper() noexcept(false)
    {
        this->impl.disconnect();
    }

    operator Transport& ()
    {
        return impl;
    }

    ImplTransport impl;
};

struct GeneratorTransport
{
    GeneratorTransport(bytes_view buffer);

    ~GeneratorTransport() noexcept(false);

    void disable_remaining_error()
    {
        this->impl.disable_remaining_error();
    }

    operator InTransport ()
    {
        return impl;
    }

    operator Transport& ()
    {
        return impl;
    }

    Transport* operator->()
    {
        return &impl;
    }

    struct Impl : Transport
    {
        Impl(bytes_view buffer);

        bool disconnect() override;

        void disable_remaining_error()
        {
            this->remaining_is_error = false;
        }

    private:
        Read do_atomic_read(uint8_t * buffer, size_t len) override;

        size_t do_partial_read(uint8_t* buffer, size_t len) override;

        const std::unique_ptr<uint8_t[]> data;
        const std::size_t len;
        std::size_t current = 0;
        bool remaining_is_error = true;

        friend class GeneratorTransport;
    };

private:
    Impl impl;
};


struct BufTransport : SequencedTransport
{
    std::string buf;

    [[nodiscard]] std::size_t size() const noexcept { return buf.size(); }
    void clear() noexcept { buf.clear(); }
    std::string& data() noexcept { return buf; }

    bool next() override
    {
        return true;
    }

private:
    void do_send(const uint8_t * const data, size_t len) override;

    Read do_atomic_read(uint8_t * buffer, size_t len) override;
    size_t do_partial_read(uint8_t* buffer, size_t len) override;
};


struct CheckTransport
{
    CheckTransport(buffer_view buffer);

    size_t remaining() const
    {
        return this->impl.remaining();
    }

    ~CheckTransport() noexcept(false);

    void disable_remaining_error()
    {
        this->impl.disable_remaining_error();
    }

    operator OutTransport ()
    {
        return impl;
    }

    operator SequencedTransport& ()
    {
        return impl;
    }

    SequencedTransport* operator->()
    {
        return &impl;
    }

    struct Impl : SequencedTransport
    {
        Impl(buffer_view buffer);

        bool disconnect() override;

        bool next() override
        {
            return true;
        }

        size_t remaining() const
        {
            return this->len - this->current;
        }

        void disable_remaining_error()
        {
            this->remaining_is_error = false;
        }

    private:
        void do_send(const uint8_t * const data, size_t len) override;

        std::unique_ptr<uint8_t[]> data;
        std::size_t len;
        std::size_t current = 0;
        bool remaining_is_error = true;

        friend class CheckTransport;
    };

private:
    Impl impl;
};


struct TestTransport
{
    TestTransport(bytes_view indata, bytes_view outdata);

    ~TestTransport() noexcept(false);

    void disable_remaining_error()
    {
        this->impl.check.disable_remaining_error();
        this->impl.gen.disable_remaining_error();
    }

    size_t remaining() const
    {
        return this->impl.check.remaining();
    }

    void set_public_key(bytes_view key);

    operator InTransport ()
    {
        return impl;
    }

    operator OutTransport ()
    {
        return impl;
    }

    operator Transport& ()
    {
        return impl;
    }

    Transport* operator->()
    {
        return &impl;
    }

    struct Impl : public Transport
    {
        Impl(bytes_view indata, bytes_view outdata);

        TlsResult enable_server_tls(const char * /*certificate_password*/, const TlsConfig & /*tls_config*/) override
        {
            return TlsResult::Ok;
        }

        [[nodiscard]] u8_array_view get_public_key() const override;

    protected:
        Read do_atomic_read(uint8_t * buffer, size_t len) override;
        size_t do_partial_read(uint8_t* buffer, size_t len) override;

        void do_send(const uint8_t * const buffer, size_t len) override;

    private:
        CheckTransport::Impl check;
        GeneratorTransport::Impl gen;
        std::unique_ptr<uint8_t[]> public_key;
        std::size_t public_key_length = 0;

        friend class TestTransport;
    };

private:
    Impl impl;
};


struct MemoryTransport
{
    ~MemoryTransport() noexcept(false);

    void disable_remaining_error()
    {
        this->impl.remaining_is_error = false;
    }

    operator SequencedTransport& ()
    {
        return impl;
    }

    SequencedTransport* operator->()
    {
        return &impl;
    }

private:
    struct Impl : public SequencedTransport
    {
        bool disconnect() override;

        bool next() override
        {
            return true;
        }

    private:
        Read do_atomic_read(uint8_t * buffer, size_t len) override;

        size_t do_partial_read(uint8_t* buffer, size_t len) override;

        void do_send(const uint8_t * const buffer, size_t len) override;

        uint8_t buf[65536];
        bool remaining_is_error = true;
        InStream  in_stream{buf};
        OutStream out_stream{buf};

        friend class MemoryTransport;
    };

    Impl impl;
};
