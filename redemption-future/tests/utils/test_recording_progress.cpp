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
   Copyright (C) Wallix 2021
   Author(s): Proxies team
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"


#include "utils/recording_progress.hpp"
#include "utils/fileutils.hpp"


using namespace std::literals::chrono_literals;

RED_AUTO_TEST_CASE_WF(TestRecordingProgress, wf)
{
    auto const start_time = MonotonicTimePoint(123456789s);
    auto const end_time = start_time + 100s;
    auto const read_permisssions = FilePermissions::user_permissions(BitPermissions::read);
    char const * filename = wf.c_str();

    auto read_file = [=, contents = std::string()] () mutable -> std::string const& {
        contents.clear(); (void)append_file_contents(filename, contents);
        return contents;
    };

    auto mk_progress = [=]{
        unlink(filename);
        return UpdateProgressData(filename, read_permisssions, start_time, end_time);
    };


    {
        auto p = mk_progress();

        RED_REQUIRE(p.is_valid());

        RED_CHECK_EQUAL(read_file(), R"({"percentage":0,"eta":-1,"videos":0})");

        p(start_time + 10s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":10,"eta":0,"videos":0})");

        p(start_time + 90s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":90,"eta":0,"videos":0})");

        p(start_time + 100s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":99,"eta":0,"videos":0})");
    }
    RED_CHECK_EQUAL(read_file(), R"({"percentage":100,"eta":0,"videos":1})");

    {
        auto p = mk_progress();

        RED_REQUIRE(p.is_valid());

        RED_CHECK_EQUAL(read_file(), R"({"percentage":0,"eta":-1,"videos":0})");

        p(start_time + 10s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":10,"eta":0,"videos":0})");

        p.next_video(start_time + 90s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":90,"eta":0,"videos":1})");

        p(start_time + 100s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":99,"eta":0,"videos":1})");
    }
    RED_CHECK_EQUAL(read_file(), R"({"percentage":100,"eta":0,"videos":2})");

    {
        auto p = mk_progress();

        RED_REQUIRE(p.is_valid());

        RED_CHECK_EQUAL(read_file(), R"({"percentage":0,"eta":-1,"videos":0})");

        p(start_time + 10s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":10,"eta":0,"videos":0})");

        p.next_video(start_time + 90s);
        RED_CHECK_EQUAL(read_file(), R"({"percentage":90,"eta":0,"videos":1})");

        p.raise_error(2, "plouf"_sized_av);
        RED_CHECK_EQUAL(read_file(),
            R"({"percentage":90,"eta":0,"videos":1,"error":{"code":2,"message":"plouf"}})");
    }
    RED_CHECK_EQUAL(read_file(), R"({"percentage":90,"eta":0,"videos":1,"error":{"code":2,"message":"plouf"}})");
}
