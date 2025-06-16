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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test to region object
   Using lib boost functions, some tests need to be added

*/


#include "test_only/test_framework/redemption_unit_tests.hpp"


#include "utils/random.hpp"
#include "system/urandom.hpp"
#include "test_only/lcg_random.hpp"


RED_AUTO_TEST_CASE(TestUdevRandom)
{
    URandom rnd;
    unsigned mem[128] = {};
    rnd.random(writable_bytes_view::from_raw_object(mem));
    unsigned count_null = 0;
    for (unsigned int i : mem){
        if (!i) {
            count_null++;
        }
    }
    // well, theoratically as we are testing a random generator,
    // this test may possibly fail and even generator yield as many zeroes
    // as we (dont) want.
    // however this should be quite rare.
    // so, if the test fails, I guess we should just run it again
    RED_CHECK(count_null == 0);

    unsigned mem2[1024] = {};

    rnd.random(writable_bytes_view::from_raw_object(mem2));
    RED_CHECK(memcmp(mem, mem2, sizeof(mem)) != 0);

    unsigned mem3[1024] = {};

    rnd.random(writable_bytes_view::from_raw_object(mem3));
    RED_CHECK(memcmp(mem2, mem3, sizeof(mem)) != 0);
    RED_CHECK(memcmp(mem, mem3, sizeof(mem)) != 0);
}

RED_AUTO_TEST_CASE(TestLCGRandom)
{
    LCGRandom rnd;
    unsigned mem[128] = {};
    rnd.random(writable_bytes_view::from_raw_object(mem));
    unsigned count_null = 0;
    for (unsigned int i : mem){
        if (!i) { count_null++; }
    }
    // well, theoratically as we are testing a random generator,
    // this test may possibly fail and even generator yield as many zeroes
    // as we (dont) want.
    // however this should be quite rare.
    // so, if the test fails, I guess we should just run it again
    RED_CHECK(count_null == 0);

    unsigned mem2[1024] = {};
    rnd.random(writable_bytes_view::from_raw_object(mem2));

    RED_CHECK(memcmp(mem, mem2, sizeof(mem)) != 0);
}
