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
    Copyright (C) Wallix 2014
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "core/error.hpp"

RED_AUTO_TEST_CASE(TestError)
{
    Error e(ERR_SSL_CALL_FAILED);

    RED_CHECK_EQUAL("Exception ERR_SSL_CALL_FAILED no: 25000", e.errmsg());

    RED_CHECK_EQUAL("Exception ERR_SSL_CALL_FAILED", e.errmsg(false));

    RED_CHECK_EQUAL("Exception ERR_SSL_CALL_FAILED no: 25000", e.errmsg());
}
