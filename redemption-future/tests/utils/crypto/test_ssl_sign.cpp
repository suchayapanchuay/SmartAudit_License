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
   Author(s): Christophe Grosjean, Meng Tan
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "utils/crypto/ssl_sign.hpp"

RED_AUTO_TEST_CASE(TestSign)
{
    Sign hmac("key"_av);

    hmac.update("The quick brown fox jumps over the lazy dog"_av);

    uint8_t sig[16];
    hmac.final(make_writable_sized_array_view(sig));
    RED_CHECK(make_array_view(sig) ==
        "\x10\xfb\x60\x2c\xef\xe7\xe0\x0b\x91\xc2\xe2\x12\x39\x80\xe1\x94"_av);
}
