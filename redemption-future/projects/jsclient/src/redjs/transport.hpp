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

#pragma once

#include "transport/transport.hpp"
#include "utils/sugar/bytes_view.hpp"

#include <string>
#include <vector>

#include <cstdint>


namespace redjs
{

struct Transport : ::Transport
{
    TlsResult enable_client_tls(
        CertificateChecker certificate_checker,
        const TlsConfig & tls_config, AnonymousTls anonymous_tls) override;

    size_t do_partial_read(uint8_t * data, size_t len) override;

    void do_send(const uint8_t * buffer, size_t len) override;

    int get_fd() const override;

    void push_input_buffer(std::string&& data);

    bytes_view get_output_buffer() const noexcept;

    void clear_output_buffer() noexcept;

private:
    std::vector<std::string> input_buffers;
    std::vector<uint8_t> output_buffer;
    std::size_t current_pos = 0;
};

}
