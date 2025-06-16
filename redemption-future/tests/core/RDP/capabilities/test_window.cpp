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

   Unit test to RDP Rail object
   Using lib boost functions for testing
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"


#include "core/RDP/capabilities/window.hpp"

RED_AUTO_TEST_CASE(TestCapabilityWindowListEmit)
{
    WindowListCaps windowlist_caps;
    windowlist_caps.WndSupportLevel = TS_WINDOW_LEVEL_SUPPORTED_EX;
    windowlist_caps.NumIconCaches = 255;
    windowlist_caps.NumIconCacheEntries = 65535;

    RED_CHECK_EQUAL(windowlist_caps.capabilityType, static_cast<uint16_t>(CAPSTYPE_WINDOW));
    RED_CHECK_EQUAL(windowlist_caps.len, static_cast<uint16_t>(CAPLEN_WINDOW));
    RED_CHECK_EQUAL(windowlist_caps.WndSupportLevel, static_cast<uint32_t>(2));
    RED_CHECK_EQUAL(windowlist_caps.NumIconCaches, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(windowlist_caps.NumIconCacheEntries, static_cast<uint16_t>(65535));

    StaticOutStream<1024> out_stream;
    windowlist_caps.emit(out_stream);

    InStream stream(out_stream.get_produced_bytes());

    WindowListCaps windowslist_caps2;

    RED_CHECK_EQUAL(windowslist_caps2.capabilityType, static_cast<uint16_t>(CAPSTYPE_WINDOW));
    RED_CHECK_EQUAL(windowslist_caps2.len, static_cast<uint16_t>(CAPLEN_WINDOW));

    RED_CHECK_EQUAL(static_cast<uint16_t>(CAPSTYPE_WINDOW), stream.in_uint16_le());
    RED_CHECK_EQUAL(static_cast<uint16_t>(CAPLEN_WINDOW), stream.in_uint16_le());
    windowslist_caps2.recv(stream, CAPLEN_WINDOW);

    RED_CHECK_EQUAL(windowslist_caps2.WndSupportLevel, static_cast<uint32_t>(2));
    RED_CHECK_EQUAL(windowslist_caps2.NumIconCaches, static_cast<uint8_t>(255));
    RED_CHECK_EQUAL(windowslist_caps2.NumIconCacheEntries, static_cast<uint16_t>(65535));
}
