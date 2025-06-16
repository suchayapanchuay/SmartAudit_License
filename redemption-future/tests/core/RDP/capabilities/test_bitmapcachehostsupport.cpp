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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean

   Unit test to RDP BitmapCacheHostSupport object
   Using lib boost functions for testing
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "core/RDP/capabilities/bitmapcachehostsupport.hpp"


RED_AUTO_TEST_CASE(TestCapabilityBitmapCacheHostSupportsEmit)
{
    BitmapCacheHostSupportCaps bitmapcachehostsupport_caps;
    bitmapcachehostsupport_caps.cacheVersion = 255;
    bitmapcachehostsupport_caps.pad1 = 255;
    bitmapcachehostsupport_caps.pad2 = 65535;

    RED_CHECK_EQUAL(bitmapcachehostsupport_caps.capabilityType, CAPSTYPE_BITMAPCACHE_HOSTSUPPORT);
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps.len, static_cast<uint16_t>(CAPLEN_BITMAPCACHE_HOSTSUPPORT));
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps.cacheVersion, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps.pad1, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps.pad2, static_cast<uint16_t>(65535));

    StaticOutStream<1024> out_stream;
    bitmapcachehostsupport_caps.emit(out_stream);
    InStream stream(out_stream.get_produced_bytes());

    BitmapCacheHostSupportCaps bitmapcachehostsupport_caps2;

    RED_CHECK_EQUAL(bitmapcachehostsupport_caps2.capabilityType, CAPSTYPE_BITMAPCACHE_HOSTSUPPORT);
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps2.len, static_cast<uint16_t>(CAPLEN_BITMAPCACHE_HOSTSUPPORT));

    RED_CHECK_EQUAL(static_cast<uint16_t>(CAPSTYPE_BITMAPCACHE_HOSTSUPPORT), stream.in_uint16_le());
    RED_CHECK_EQUAL(static_cast<uint16_t>(CAPLEN_BITMAPCACHE_HOSTSUPPORT), stream.in_uint16_le());
    bitmapcachehostsupport_caps2.recv(stream, CAPLEN_BITMAPCACHE_HOSTSUPPORT);

    RED_CHECK_EQUAL(bitmapcachehostsupport_caps2.cacheVersion, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps2.pad1, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(bitmapcachehostsupport_caps2.pad2, static_cast<uint16_t>(65535));
}
