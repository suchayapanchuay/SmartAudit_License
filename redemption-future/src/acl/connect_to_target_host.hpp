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
  Copyright (C) Wallix 2010
  Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
             Raphael Zhou, Meng Tan

  Connect to target Underlying socket for module needs
*/

#pragma once

#include "acl/auth_api.hpp"
#include "configs/config.hpp"
#include "core/log_id.hpp"
#include "translation/trkeys.hpp"
#include "utils/log.hpp"
#include "utils/sugar/unique_fd.hpp"
#include "utils/log_siem.hpp"
#include "utils/netutils.hpp"
#include "utils/strutils.hpp"
#include "utils/error_message_ctx.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


inline unique_fd connect_to_target_host(
    Inifile & ini, SessionLogApi& session_log, ErrorMessageCtx& err_msg_ctx,
    TrKey authentification_fail, bool enable_ipv6,
    std::chrono::milliseconds connection_establishment_timeout,
    std::chrono::milliseconds tcp_user_timeout)
{
    auto throw_error = [&ini, &session_log, &err_msg_ctx](char const* error_message, int id) {
        log_siem::target_connection_failed(
            ini.get<cfg::globals::target_user>().c_str(),
            ini.get<cfg::context::session_id>().c_str(),
            ini.get<cfg::context::target_host>().c_str(),
            ini.get<cfg::context::target_port>(),
            error_message);

        session_log.log6(LogId::CONNECTION_FAILED, {});

        err_msg_ctx.set_msg(trkeys::target_fail);

        LOG(LOG_ERR, "%s", (id == 1)
            ? "Failed to connect to remote TCP host (1)"
            : "Failed to connect to remote TCP host (2)");
        throw Error(ERR_SOCKET_CONNECT_FAILED);
    };

    log_siem::target_connection(
        ini.get<cfg::globals::target_user>().c_str(),
        ini.get<cfg::context::session_id>().c_str(),
        ini.get<cfg::context::target_host>().c_str(),
        ini.get<cfg::context::target_port>());

    const char *ip = ini.get<cfg::context::target_host>().c_str();
    int port = ini.get<cfg::context::target_port>();
    const char *error_message = nullptr;
    char resolved_ip_addr[NI_MAXHOST] { };
    unique_fd client_sck = invalid_fd();

    // Handle ipv4 or both ipv4 and ipv6
    if (!enable_ipv6)
    {
        in_addr s4_sin_addr;

        if ((error_message = resolve_ipv4_address(ip, s4_sin_addr)))
        {
            // TODO: actually this is DNS Failure or invalid address
            throw_error(error_message, 1);
        }

        utils::strlcpy(resolved_ip_addr, inet_ntoa(s4_sin_addr));
        client_sck = ip_connect_ipv4(ip,
                                     port,
                                     connection_establishment_timeout,
                                     tcp_user_timeout,
                                     &error_message);
    }
    else
    {
        AddrInfoPtrWithDel_t addr_info_ptr =
            resolve_both_ipv4_and_ipv6_address(ip, port, &error_message);

        if (error_message)
        {
            throw_error(error_message, 1);
        }

        if (int res = ::getnameinfo(addr_info_ptr->ai_addr,
                                    addr_info_ptr->ai_addrlen,
                                    resolved_ip_addr,
                                    sizeof(resolved_ip_addr),
                                    nullptr,
                                    0,
                                    NI_NUMERICHOST))
        {
            error_message = (res == EAI_SYSTEM) ?
                ::strerror(errno) : ::gai_strerror(res);
            throw_error(error_message, 1);
        }

        client_sck = ip_connect(ip,
                                port,
                                connection_establishment_timeout,
                                tcp_user_timeout,
                                &error_message);
    }

    if (!client_sck.is_open())
    {
        throw_error(error_message, 2);
    }

    err_msg_ctx.set_msg(authentification_fail);
    ini.set<cfg::context::ip_target>(resolved_ip_addr);
    return client_sck;
}
