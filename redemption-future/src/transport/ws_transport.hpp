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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan

   Transport layer abstraction, socket implementation with TLS support
*/

#pragma once

#include "transport/socket_transport.hpp"

class WsTransport final : public SocketTransport
{
public:
    enum UseTls : bool { No, Yes };

    struct TlsOptions
    {
        std::string certificate_password {};
        TlsConfig tls_config;
    };

    WsTransport(
        Name name, unique_fd sck, chars_view ip_address, int port,
        std::chrono::milliseconds connection_establishment_timeout,
        std::chrono::milliseconds tcp_user_timeout,
        std::chrono::milliseconds recv_timeout,
        UseTls use_tls, TlsOptions tls_options,
        Verbose verbose);

    bool disconnect() override;

protected:
    size_t do_partial_read(uint8_t * buffer, size_t len) override;

    [[noreturn]]
    Read do_atomic_read(uint8_t * buffer, size_t len) override;

    void do_send(const uint8_t * const buffer, size_t len) override;

    TlsResult enable_client_tls(
        CertificateChecker certificate_checker,
        TlsConfig const& tls_config, AnonymousTls anonymous_tls) override;

private:
    class D;

    enum class State : char;

    State state;

    TlsOptions tls_options;
};
