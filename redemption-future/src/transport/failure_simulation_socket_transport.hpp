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

class FailureSimulationSocketTransport final : public SocketTransport
{
public:
    FailureSimulationSocketTransport(
      bool simulate_read_failure,
      Name name,
      unique_fd sck,
      chars_view ip_address,
      int port,
      std::chrono::milliseconds connection_establishment_timeout,
      std::chrono::milliseconds tcp_user_timeout,
      std::chrono::milliseconds recv_timeout,
      Verbose verbose)
    : SocketTransport(name,
                      std::move(sck),
                      ip_address, port,
                      connection_establishment_timeout,
                      tcp_user_timeout,
                      recv_timeout,
                      verbose)
    , simulate_read_failure(simulate_read_failure)
    {}

private:
    size_t do_partial_read(uint8_t * buffer, size_t len) override
    {
        if (this->simulate_read_failure)
        {
            if (this->current_successful_operations >= this->max_successful_read_operations)
            {
                LOG(LOG_ERR, "FailureSimulationSocketTransport::do_partial_read: Simulates read error");
                throw Error(ERR_TRANSPORT_NO_MORE_DATA, 0, this->get_fd());
            }

            this->current_successful_operations++;
        }

        return SocketTransport::do_partial_read(buffer, len);
    }

    Read do_atomic_read(uint8_t * buffer, size_t len) override
    {
        if (this->simulate_read_failure)
        {
            if (this->current_successful_operations >= this->max_successful_read_operations)
            {
                LOG(LOG_ERR, "FailureSimulationSocketTransport::do_atomic_read: Simulates read error");
                throw Error(ERR_TRANSPORT_NO_MORE_DATA, 0, this->get_fd());
            }

            this->current_successful_operations++;
        }

        return SocketTransport::do_atomic_read(buffer, len);
    }

    void do_send(const uint8_t * const buffer, size_t len) override
    {
        if (!this->simulate_read_failure)
        {
            if (this->current_successful_operations >= this->max_successful_write_operations)
            {
                LOG(LOG_ERR, "FailureSimulationSocketTransport::do_send: Simulates write error");
                throw Error(ERR_TRANSPORT_WRITE_FAILED, 0, this->get_fd());
            }

            this->current_successful_operations++;
        }

        SocketTransport::do_send(buffer, len);
    }

private:
    const bool simulate_read_failure;

    static constexpr size_t max_successful_read_operations = 800;

    static constexpr size_t max_successful_write_operations = 2000;

    size_t current_successful_operations = 0;
};
