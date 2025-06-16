/*
SPDX-FileCopyrightText: 2024 Wallix Proxies Team
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "utils/snprintf_av.hpp"

using namespace std::string_view_literals;


RED_AUTO_TEST_CASE(TestSNPrintfAV)
{
    char raw_buf[10];
    auto buf = make_writable_array_view(raw_buf);

    RED_CHECK(snprintf_av(buf, "%s:%s", "abc", "def") == "abc:def"_av);
    RED_CHECK(snprintf_av(buf, "%s:%s:%s", "abc", "def", "g") == "abc:def:g"_av);
    RED_CHECK(snprintf_av(buf, "%s:%s:%s", "abc", "def", "ghi") == "abc:def:g"_av);

    RED_CHECK(SNPrintf<10>("%s:%s", "abc", "def").str() == "abc:def"_av);
    RED_CHECK(SNPrintf<10>("%s:%s:%s", "abc", "def", "g").str() == "abc:def:g"_av);
    RED_CHECK(SNPrintf<10>("%s:%s:%s", "abc", "def", "ghi").str() == "abc:def:g"_av);
}
