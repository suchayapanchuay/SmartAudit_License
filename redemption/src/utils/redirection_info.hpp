/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2015
    Author(s): Christophe Grosjean, Jonathan Poelen,
               Meng Tan, Raphael Zhou
*/

#pragma once

#include "utils/log.hpp"
#include "utils/hexdump.hpp"
#include <vector>


struct RedirectionInfo {
    bool valid = false;
    uint32_t session_id = 0;
    uint8_t host[513] {};
    uint8_t username[257] {};

    // TODO should be a static array (static_vector<512>)
    std::vector<uint8_t> password_or_cookie;

    uint8_t domain[257] {};
    uint8_t lb_info[513] {};
    uint32_t lb_info_length = 0;
    bool dont_store_username = false;
    bool server_tsv_capable = false;
    bool smart_card_logon = false;

    bool host_is_fqdn = false;

    RedirectionInfo() = default;

    void log(int level, const char * message) const {
        LOG(level
            , "%s: RedirectionInfo(valid=%s, session_id=%u, host='%s', username='%s',"
            " password_or_cookie='%s', domain='%s', LoadBalanceInfoLength=%u,"
            " dont_store_username=%s, server_tsv_capable=%s, smart_card_logon=%s)"
            , message
            , this->valid?"true":"false"
            , this->session_id
            , this->host
            , this->username
            , this->password_or_cookie.empty() ? "<null>" : "<hidden>"
            , this->domain
            , this->lb_info_length
            , this->dont_store_username?"true":"false"
            , this->server_tsv_capable?"true":"false"
            , this->smart_card_logon?"true":"false"
            );
        if (this->lb_info_length > 0) {
            hexdump_c(this->lb_info, this->lb_info_length);
        }
    }
};
