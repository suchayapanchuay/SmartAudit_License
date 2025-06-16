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
   Copyright (C) Wallix 2018
   Author(s): David Fort

   A proxy that will capture all the traffic to the target
*/

#pragma once

#include "utils/fixed_random.hpp"
#include "core/RDP/nego.hpp"
#include "proxy_recorder/nla_tee_transport.hpp"
#include "proxy_recorder/extract_user_domain.hpp"

class NegoClient
{
    FixedRandom random;
    Transport & trans;
    RdpNego nego;

public:
    NegoClient(
        bool is_nla, bool is_admin_mode, Transport& trans, const TimeBase & time_base,
        char const* host, std::string_view target_user, char const* password,
        bool enable_kerberos, const TlsConfig & tls_config, RdpNego::Verbose verbosity
    )
    : trans(trans)
    , nego(target_user, host, is_nla, enable_kerberos, true, true, true,
        is_admin_mode, this->random, time_base, tls_config, verbosity)
    {
        auto [username, domain] = extract_user_domain(target_user);
        nego.set_identity(username, password, domain, "ProxyRecorder"_av);
        // static char ln_info[] = "tsv://MS Terminal Services Plugin.1.Sessions\x0D\x0A";
        // nego.set_lb_info(byte_ptr_cast(ln_info), sizeof(ln_info)-1);
    }

    void send_negotiation_request()
    {
        this->nego.send_negotiation_request(this->trans);
    }

    bool recv_next_data(TpduBuffer& tpdu_buffer, Transport::CertificateChecker certificate_checker)
    {
        return this->nego.recv_next_data(tpdu_buffer, this->trans, certificate_checker);
    }
};
