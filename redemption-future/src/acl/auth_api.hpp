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
    Copyright (C) Wallix 2017
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#pragma once

#include "utils/sugar/noncopyable.hpp"
#include "utils/sugar/array_view.hpp"
#include "acl/acl_report.hpp"

struct AclReportApi : noncopyable
{
    virtual void acl_report(AclReport report) = 0;

    virtual ~AclReportApi() = default;
};

struct NullAclReport final : AclReportApi
{
    void acl_report(AclReport report) override
    {
        (void)report;
    }
};


enum class LogId : uint16_t;

struct KVLog
{
    chars_view key;
    chars_view value;

    KVLog() = default;

    KVLog(chars_view key, chars_view value) noexcept : key(key), value(value) {}
};

struct KVLogList : array_view<KVLog>
{
    KVLogList(array_view<KVLog> kv_list) noexcept : array_view<KVLog>(kv_list) {}
    KVLogList(std::initializer_list<KVLog> kv_list) noexcept : array_view<KVLog>(kv_list) {}
};

struct SessionLogApi : AclReportApi
{
    virtual void log6(LogId id, KVLogList kv_list) = 0;
    // empty name for disable
    virtual void set_control_owner_ctx(chars_view name) = 0;
};

struct NullSessionLog : SessionLogApi
{
    void acl_report(AclReport report) override
    {
        (void)report;
    }

    void log6(LogId id, KVLogList kv_list) override
    {
        (void)id;
        (void)kv_list;
    }

    void set_control_owner_ctx(chars_view name) override
    {
        (void)name;
    }
};
