/*
SPDX-FileCopyrightText: 2024 Wallix Proxies Team
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "acl/acl_update_session_report.hpp"

#include <string_view>

RED_AUTO_TEST_CASE(TestAclReportBuilder)
{
    std::string s;
    AclUpdateSessionReport acl_update_session_report;
    auto acl_report = [&](bool first_call, AclReport report, std::string_view target_device) -> std::string& {
        acl_update_session_report.update_report(OutParam{s}, first_call, report, target_device);
        return s;
    };

    RED_CHECK(acl_report(true, AclReport::file_system_full(), "target1")
        == "FILESYSTEM_FULL:target1:"_av);
    RED_CHECK(acl_report(true, AclReport::close_session_successful(), "target2")
        == "CLOSE_SESSION_SUCCESSFUL:target2:OK."_av);
    RED_CHECK(acl_report(false, AclReport::file_system_full(), "target3")
        == "CLOSE_SESSION_SUCCESSFUL:target2:OK.\x01""FILESYSTEM_FULL:target3:"_av);
    // already set, ignored
    RED_CHECK(acl_report(false, AclReport::close_session_successful(), "target4")
        == "CLOSE_SESSION_SUCCESSFUL:target2:OK.\x01""FILESYSTEM_FULL:target3:"_av);
    RED_CHECK(acl_report(true, AclReport::session_event("msg1"_av), "target5")
        == "SESSION_EVENT:target5:msg1"_av);

    // \x01 whitin message -> removed
    RED_CHECK(acl_report(true, AclReport::session_event("msg\x01_1"_av), "target5")
        == "SESSION_EVENT:target5:msg_1"_av);
    RED_CHECK(acl_report(false, AclReport::session_event("msg\x01\x01_\x01_2"_av), "target6")
        == "SESSION_EVENT:target5:msg_1\x01SESSION_EVENT:target6:msg__2"_av);
}
