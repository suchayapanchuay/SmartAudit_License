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
Copyright (C) Wallix 2010-2018
Author(s): Jonathan Poelen
*/

#ifdef __EMSCRIPTEN__
#define BOOST_TEST_DISABLE_ALT_STACK
extern "C" inline int sigaction(
    int /*signum*/, const struct sigaction */*act*/, struct sigaction*/*oldact*/)
{
    return 0;
};
#endif

// #define BOOST_TEST_ALTERNATIVE_INIT_API
#include "cxx/diagnostic.hpp"

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_GCC_ONLY_IGNORE("-Wnull-dereference")

#include <boost/test/unit_test_monitor.hpp>
#include <boost/test/included/unit_test.hpp>

REDEMPTION_DIAGNOSTIC_POP()
