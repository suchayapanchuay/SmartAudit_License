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
   Copyright (C) Wallix 2016
   Author(s): Jennifer Inthavong

   T.124 Generic Conference Control (GCC) Unit Test
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "core/RDP/gcc/userdata/cs_mcs_msgchannel.hpp"


RED_AUTO_TEST_CASE(Test_gcc_user_data_cs_mcs_msgchannel)
{
    InStream stream(
        "\x06\xc0"         // CS_MCS_MSGCHANNEL
        "\x08\x00"         // 8 bytes user Data

        "\x00\x00\x00\x00" // TS_UD_CS_MCS_MSGCHANNEL::flags
        ""_av);
    GCC::UserData::CSMCSMsgChannel cs_mcs_msgchannel;
    cs_mcs_msgchannel.recv(stream);
    RED_CHECK_EQUAL(CS_MCS_MSGCHANNEL, cs_mcs_msgchannel.userDataType);
    RED_CHECK_EQUAL(8, cs_mcs_msgchannel.length);
    RED_CHECK_EQUAL(0, cs_mcs_msgchannel.flags);

    // cs_mcs_msgchannel.log("Client Received");
}
