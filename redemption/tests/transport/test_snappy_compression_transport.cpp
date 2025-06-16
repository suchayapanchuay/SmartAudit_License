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
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "transport/snappy_compression_transport.hpp"
#include "test_only/transport/test_transport.hpp"

#include <string_view>

using namespace std::string_view_literals;


RED_AUTO_TEST_CASE(TestSnappyCompressionTransport)
{
    MemoryTransport mt;

    {
        SnappyCompressionOutTransport out_trans(mt);

        RED_CHECK_NO_THROW(out_trans.send( "azert" "azert" "azert" "azert" , 21));
        RED_CHECK_NO_THROW(out_trans.send( "wallix" "wallix" "wallix" "wallix" "wallix", 31));
        RED_CHECK_NO_THROW(out_trans.next());
        RED_CHECK_NO_THROW(out_trans.send(
            "0123456789ABCDEF" "0123456789ABCDEF" "0123456789ABCDEF" "0123456789ABCDEF", 65));
    }

    {
        SnappyCompressionInTransport  in_trans(mt);

        char   in_data[128] = { 0 };

        RED_CHECK_NO_THROW(in_trans.recv_boom(in_data, 21));
        RED_CHECK(in_data == "azert" "azert" "azert" "azert"sv);

        RED_CHECK_NO_THROW(in_trans.recv_boom(in_data, 31));
        RED_CHECK(in_data == "wallix" "wallix" "wallix" "wallix" "wallix"sv);

        RED_CHECK_NO_THROW(in_trans.recv_boom(in_data, 65));
        RED_CHECK(in_data == "0123456789ABCDEF" "0123456789ABCDEF" "0123456789ABCDEF" "0123456789ABCDEF"sv);
    }
}
