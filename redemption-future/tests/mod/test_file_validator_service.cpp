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
   Copyright (C) Wallix 2019
   Author(s): Clément Moroldo
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"
#include "test_only/test_framework/file.hpp"
#include "test_only/transport/test_transport.hpp"

#include "mod/file_validator_service.hpp"


RED_AUTO_TEST_CASE(file_validatorSendFile)
{
    BufTransport trans;
    FileValidatorService file_validator{trans, FileValidatorService::Verbose::No};

    auto filename = FIXTURES_PATH "/test_infile.txt";
    auto content = RED_REQUIRE_GET_FILE_CONTENTS(filename);

    FileValidatorId id = file_validator.open_file(filename, "clamav");
    RED_CHECK(id == FileValidatorId(1));

    file_validator.send_data(id, content);
    file_validator.send_eof(id);

    const auto data_ref =
        "\x07\x00\x00\x00\x43\x00\x00\x00\x01\x00\x06\x63\x6c\x61\x6d\x61" //....C......clama !
        "\x76\x00\x01\x00\x08\x66\x69\x6c\x65\x6e\x61\x6d\x65\x00\x29\x2e" //v....filename.). !
        "\x2f\x74\x65\x73\x74\x73\x2f\x69\x6e\x63\x6c\x75\x64\x65\x73\x2f" ///tests/includes/ !
        "\x66\x69\x78\x74\x75\x72\x65\x73\x2f\x74\x65\x73\x74\x5f\x69\x6e" //fixtures/test_in !
        "\x66\x69\x6c\x65\x2e\x74\x78\x74\x01\x00\x00\x00\x1c\x00\x00\x00" //file.txt........ !
        "\x01\x57\x65\x20\x72\x65\x61\x64\x20\x77\x68\x61\x74\x20\x77\x65" //.We read what we !
        "\x20\x70\x72\x6f\x76\x69\x64\x65\x21\x03\x00\x00\x00\x04\x00\x00" // provide!....... !
        "\x00\x01"                                                         //.. !
        ""_av;
    RED_CHECK(trans.data() == data_ref);

    RED_CHECK(file_validator.open_file(filename, "clamav") == FileValidatorId(2));
}

RED_AUTO_TEST_CASE(file_validatorSendText)
{
    BufTransport trans;
    FileValidatorService file_validator{trans, FileValidatorService::Verbose::No};

    FileValidatorId id = file_validator.open_text(0x12345678, "clamav");
    RED_CHECK(id == FileValidatorId(1));

    file_validator.send_data(id, "papa pas à pou"_av);
    file_validator.send_eof(id);

    const auto data_ref =
        "\x07\x00\x00\x00\x2e\x00\x00\x00\x01\x00\x06\x63\x6c\x61\x6d\x61" //...........clama !
        "\x76\x00\x01\x00\x13\x6d\x69\x63\x72\x6f\x73\x6f\x66\x74\x5f\x6c" //v....microsoft_l !
        "\x6f\x63\x61\x6c\x65\x5f\x69\x64\x00\x09\x33\x30\x35\x34\x31\x39" //ocale_id..305419 !
        "\x38\x39\x36\x01\x00\x00\x00\x13\x00\x00\x00\x01\x70\x61\x70\x61" //896.........papa !
        "\x20\x70\x61\x73\x20\xc3\xa0\x20\x70\x6f\x75\x03\x00\x00\x00\x04" // pas .. pou..... !
        "\x00\x00\x00\x01"                                                 //.... !
        ""_av;
    RED_CHECK(trans.data() == data_ref);

    RED_CHECK(file_validator.open_file("filename", "clamav") == FileValidatorId(2));
}

RED_AUTO_TEST_CASE(file_validatorSendUnicode)
{
    BufTransport trans;
    FileValidatorService file_validator{trans, FileValidatorService::Verbose::No};

    FileValidatorId id = file_validator.open_unicode("clamav");
    RED_CHECK(id == FileValidatorId(1));

    file_validator.send_data(id, "a\0b\0c\0\0\0"_av);
    file_validator.send_eof(id);

    const auto data_ref =
        "\x07\x00\x00\x00\x1d\x00\x00\x00\x01\x00\x06""clamav\x00\x01\x00\x09"
        "format_id\x00\x02\x31\x33\x01\x00\x00\x00\x0c\x00\x00\x00\x01\x61\x00"
        "\x62\x00\x63\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x01"
        ""_av;
    RED_CHECK(trans.data() == data_ref);

    RED_CHECK(file_validator.open_file("filename", "clamav") == FileValidatorId(2));
}

RED_AUTO_TEST_CASE(file_validatorSendInfos)
{
    BufTransport trans;
    FileValidatorService file_validator{trans, FileValidatorService::Verbose::No};

    file_validator.send_infos({
        "key1"_av, "value1"_av,
        "key"_av, "v2"_av,
        "k3"_av, "value"_av,
    });

    const auto data_ref =
        "\x06\x00\x00\x00\x24"
        "\x00\x03"
        "\x00\x04key1" "\x00\x06value1"
        "\x00\x03key"  "\x00\x02v2"
        "\x00\x02k3"   "\x00\x05value"
        ""_av;
    RED_CHECK(trans.data() == data_ref);
}

RED_AUTO_TEST_CASE(file_validatorReceive)
{
    BufTransport trans;
    FileValidatorService file_validator{trans, FileValidatorService::Verbose::No};

    auto setbuf = [&](bytes_view data){
        trans.buf.assign(data.as_charp(), data.size());
    };

    RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);

    // message 1
    {
        auto response =
            "\x05\x00\x00\x00\x00"  // msg_type, len(ignored)
            "\x00"                  // result
            "\x00\x00\x00\x01"      // file id
            "\x00\x00\x00\x02"      // size
            ""_av;
        auto pos = response.size() / 2;
        setbuf(response.first(pos));
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
        setbuf(response.from_offset(pos));
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
        setbuf("o"_av);
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
        setbuf("k"_av);
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::HasContent);
        RED_CHECK(file_validator.last_file_id() == FileValidatorId(1));
        RED_CHECK(file_validator.get_content() == "ok");
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
    }

    // message 2
    {
        auto response =
            "\x05\x00\x00\x00\x00"  // msg_type, len(ignored)
            "\x01"                  // result
            "\x00\x00\x00\x04"      // file id
            "\x00\x00\x00\x02"      // size
            "ko"_av;
        setbuf(response);
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::HasContent);
        RED_CHECK(file_validator.last_file_id() == FileValidatorId(4));
        RED_CHECK(file_validator.get_content() == "ko");
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
    }

    // message 3
    {
        auto response =
            "\x05\x00\x00\x00\x00"  // msg_type, len(ignored)
            "\x01"                  // result
            "\x00\x00\x00\x03"      // file id
            "\x00\x00\x00\x02"      // size
            "ok"
            "\x05\x00\x00\x00\x00"  // msg_type, len(ignored)
            "\x01"                  // result
            "\x00\x00\x00\x05"      // file id
            "\x00\x00\x00\x02"      // size
            "ko"_av;
        setbuf(response);
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::HasContent);
        RED_CHECK(file_validator.last_file_id() == FileValidatorId(3));
        RED_CHECK(file_validator.get_content() == "ok");
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::HasContent);
        RED_CHECK(file_validator.last_file_id() == FileValidatorId(5));
        RED_CHECK(file_validator.get_content() == "ko");
        RED_CHECK(file_validator.receive_response() == FileValidatorService::ResponseType::WaitingData);
    }
}
