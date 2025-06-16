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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"


#include "transport/socket_transport.hpp"
#include "core/listen.hpp"
#include "utils/select.hpp"
#include "cxx/diagnostic.hpp"
#include "cxx/cxx.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wold-style-cast")
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wunreachable-code")

// This test is somewhat tricky
// The goal is to check that SocketTransport objects are working as expected
// in order to achieve that we have to
// - create a listening socket,
// - connect a client socket to it
// - accept connected client socket
// - send data at one side
// - receive it at the other side
// The problem is that we have to do everything at once and never block or fail on IO
// this is done through one select loop listening on everybody and non blocking calls
// (non blocking calls should not even be necessary thanks to select)

RED_AUTO_TEST_CASE(TestSocketTransport)
{
    int port = 4444;

    unique_fd sck_server = create_server(inet_addr("127.0.0.1"), port, EnableTransparentMode::No);

    int nb_inbuffer = 0;
    uint8_t buffer[1024] {};
    uint8_t * p = buffer;

    int nb_recv_sck = 0;
    // 10 should be enough for testing
    int recv_sck[10];
    std::unique_ptr<SocketTransport> sck_trans[10];

    bool run = true;

    const char * ip = "127.0.0.1";
    int client_sck = socket(PF_INET, SOCK_STREAM, 0);
    union
    {
      sockaddr s;
      sockaddr_storage ss;
      sockaddr_in s4;
      sockaddr_in6 s6;
    } ucs;
    memset(&ucs, 0, sizeof(ucs));
    ucs.s4.sin_family = AF_INET;
    ucs.s4.sin_port = htons(port);
    ucs.s4.sin_addr.s_addr = inet_addr(ip);
    if (ucs.s4.sin_addr.s_addr == INADDR_NONE) {
        struct addrinfo * addr_info = nullptr;
        int               result    = getaddrinfo(ip, nullptr, nullptr, &addr_info);

        if (result) {
            int          _error;
            const char * _strerror;

            if (result == EAI_SYSTEM) {
                _error    = errno;
                _strerror = strerror(errno);
            }
            else {
                _error    = result;
                _strerror = gai_strerror(result);
            }
            LOG(LOG_ERR, "DNS resolution failed for %s with errno =%d (%s)\n",
                ip, _error, _strerror);
            run = false;
        }
        ucs.s4.sin_addr.s_addr = (reinterpret_cast<sockaddr_in *>(addr_info->ai_addr))->sin_addr.s_addr;
    }
    fcntl(client_sck, F_SETFL, fcntl(client_sck, F_GETFL) | O_NONBLOCK);

    int res = -1;
    int data_sent = 0;
    std::unique_ptr<SocketTransport> client_trans;

    while (run){
        fd_set rfds;
        fd_set wfds;
        io_fd_zero(rfds);
        io_fd_zero(wfds);
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int max = sck_server.fd();
        io_fd_set(max, rfds);

        for (int i = 0 ; i < nb_recv_sck ; i++){
            if (recv_sck[i] > max){
                max = recv_sck[i];
            }
            io_fd_set(recv_sck[i], rfds);
            if (sck_trans[i]->has_tls_pending_data()) {
                timeout.tv_sec = 0;
            }
        }

        if ((client_trans && data_sent == 0) || (res == -1))
        {
            io_fd_set(client_sck, wfds);
            if (client_sck > max){
                max = client_sck;
            }
        }

        int num = select(max + 1, &rfds, &wfds, nullptr, &timeout);

        switch (num) {
        case 0:
            RED_REQUIRE_MESSAGE(timeout.tv_sec, "woke up on timeout");
            [[fallthrough]];
        default:
        {
            if (io_fd_isset(client_sck, wfds)){
                if (client_trans && (data_sent == 0)){
                    client_trans->send("AAAAXBBBBXCCCCXDDDDX", 20);
                    data_sent = 20;
                }
                else if (res == -1) {
                    res = ::connect(client_sck, &ucs.s, sizeof(ucs));
                    if (res != -1){
                        client_trans = std::make_unique<SocketTransport>(
                            "Sender"_sck_name,
                            unique_fd{client_sck},
                            "127.0.0.1"_av,
                            4444,
                            std::chrono::seconds(1),
                            std::chrono::seconds::zero(),
                            std::chrono::seconds(1),
                            SocketTransport::Verbose(0));
                    }
                }
            }

            for (int i = 0 ; i < nb_recv_sck ; i++){
                if (io_fd_isset(recv_sck[i], rfds) || sck_trans[i]->has_tls_pending_data()){
                    LOG(LOG_INFO, "activity on %d", recv_sck[i]);
                    sck_trans[i]->recv_boom(p, 5);
                    p += 5;
                    nb_inbuffer += 5;
                    LOG(LOG_INFO, "received %*s\n", nb_inbuffer, buffer);
                    if (nb_inbuffer == 20){
                        run = false;
                    }
                }
            }

            if (io_fd_isset(sck_server.fd(), rfds)){
                union
                {
                  sockaddr s;
                  sockaddr_storage ss;
                  sockaddr_in s4;
                  sockaddr_in6 s6;
                } u;
                memset(&u, 0, sizeof(u));
                u.s4.sin_family = AF_INET;
                unsigned int sin_size = sizeof(u);
                memset(&u, 0, sin_size);
                int sck = accept(sck_server.fd(), &u.s, &sin_size);
                LOG(LOG_INFO, "Incoming socket to %d (ip=%s)\n", sck, inet_ntoa(u.s4.sin_addr));
                RED_REQUIRE(sck > 0);
                recv_sck[nb_recv_sck] = sck;
                sck_trans[nb_recv_sck] = std::make_unique<SocketTransport>(
                    "Reader"_sck_name,
                    unique_fd{sck},
                    "127.0.0.1"_av,
                    4444,
                    std::chrono::milliseconds(1000),
                    std::chrono::milliseconds::zero(),
                    std::chrono::seconds(1),
                    SocketTransport::Verbose(0));
                nb_recv_sck++;
            }
        }
        break;
        case -1:
            if ((errno == EINTR)||(errno==EAGAIN)) { continue; }
            LOG(LOG_INFO, "stopped on error [%d] %s\n", num, strerror(errno));
            run = false;
        }
    }
}

REDEMPTION_DIAGNOSTIC_POP()
