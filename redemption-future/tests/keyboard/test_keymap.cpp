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
Author(s): Proxies Team
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "keyboard/keymap.hpp"
#include "keyboard/keylayouts.hpp"

namespace
{

struct DecodedKeyAndKEvent
{
    Keymap::DecodedKeys decoded_keys;
    Keymap::KEvent kevent;

    bool operator == (DecodedKeyAndKEvent const& other) const noexcept
    {
        return decoded_keys.keycode == other.decoded_keys.keycode
            && decoded_keys.flags == other.decoded_keys.flags
            && decoded_keys.uchars[0] == other.decoded_keys.uchars[0]
            && decoded_keys.uchars[1] == other.decoded_keys.uchars[1]
            && kevent == other.kevent;
    }
};

}

#if !REDEMPTION_UNIT_TEST_FAST_CHECK
# include "utils/sugar/int_to_chars.hpp"
# include "test_only/test_framework/compare_collection.hpp"

namespace
{

static ut::assertion_result test_comp_decoded(DecodedKeyAndKEvent a, DecodedKeyAndKEvent b)
{
    ut::assertion_result ar(true);

    if (REDEMPTION_UNLIKELY(!(a == b))) {
        ar = false;

        auto put = [&](std::ostream& out, DecodedKeyAndKEvent const& x){
            char uchar0[] = " (x)";
            char uchar1[] = " (x)";
            auto to_ascii = [](char* s, uint32_t uni) -> char const* {
                if (' ' <= uni && uni <= '~') {
                    s[2] = char(uni);
                    return s;
                }
                return "";
            };

            out << "{.keycode=0x" << int_to_fixed_hexadecimal_upper_zchars(underlying_cast(x.decoded_keys.keycode))
                << ", .flags=0x" << int_to_fixed_hexadecimal_upper_zchars(underlying_cast(x.decoded_keys.flags))
                << ", .uchar0=0x" << int_to_fixed_hexadecimal_upper_zchars(x.decoded_keys.uchars[0])
                << to_ascii(uchar0, x.decoded_keys.uchars[0])
                << ", .uchar1=0x" << int_to_fixed_hexadecimal_upper_zchars(x.decoded_keys.uchars[1])
                << to_ascii(uchar1, x.decoded_keys.uchars[1])
                << ", .kevent=0x" << int_to_fixed_hexadecimal_upper_zchars(underlying_cast(x.kevent))
                << "}";
        };

        auto& out = ar.message().stream();
        out << "[";
        ut::put_data_with_diff(out, a, "!=", b, put);
        out << "]";
    }

    return ar;
}

}

RED_TEST_DISPATCH_COMPARISON_EQ((), (::DecodedKeyAndKEvent), (::DecodedKeyAndKEvent), ::test_comp_decoded)
#endif


RED_AUTO_TEST_CASE(TestKeymap)
{
    // French layout
    Keymap keymap(*find_layout_by_id(KeyLayout::KbdId(0x040C)));

    using KFlags = Keymap::KbdFlags;
    using Scancode = Keymap::Scancode;
    using KV = Keymap::KEvent;
    using KeyLocks = Keymap::KeyLocks;

    using KCode = Keymap::KeyCode;

    using KeyModFlags = kbdtypes::KeyModFlags;
    using KeyMod = kbdtypes::KeyMod;

    auto event = [&](uint16_t scancode_and_flags){
        keymap.event(KFlags(scancode_and_flags & 0xff00u), Scancode(scancode_and_flags));
        return DecodedKeyAndKEvent{keymap.last_decoded_keys(), keymap.last_kevent()};
    };

    auto values = [](KCode keycode, KFlags flags, std::array<uint32_t, 2> uchars, KV kevent){
        return DecodedKeyAndKEvent{{keycode, flags, uchars}, kevent};
    };

    uint16_t release = checked_int(KFlags::Release);
    uint16_t downnnn = 0;

    // unknown scancode
    RED_CHECK_EQ(event(downnnn | 0x1AA), values(KCode(0x1AA), KFlags(0x0100), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1AA), values(KCode(0x1AA), KFlags(0x8100), {}, KV::None));

    RED_CHECK_EQ(keymap.mods().as_uint(), 0);
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown));
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown)); // auto-repeat
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown)); // auto-repeat
    RED_CHECK_EQ(event(release | 0x10 /*a*/), values(KCode(0x10), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x36 /*right shift*/), values(KCode(0x36), KFlags(), {}, KV::Shift));
    RED_CHECK_EQ(keymap.mods().as_uint(), KeyModFlags(KeyMod::RShift).as_uint());
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'A'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x10 /*a*/), values(KCode(0x10), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_ctrl_pressed());
    RED_CHECK_EQ(event(downnnn | 0x11d /*right ctrl*/), values(KCode(0x100 | 0x1d), KFlags(0x0100), {}, KV::Ctrl));
    RED_CHECK_EQ(keymap.mods().as_uint(), (KeyMod::RShift | KeyMod::RCtrl).as_uint());
    RED_CHECK(keymap.is_ctrl_pressed());
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x11d /*right ctrl*/), values(KCode(0x100 | 0x1d), KFlags(0x8100), {}, KV::None));
    RED_CHECK_EQ(keymap.mods().as_uint(), KeyModFlags(KeyMod::RShift).as_uint());
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'A'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x10 /*a*/), values(KCode(0x10), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x02 /*&*/), values(KCode(0x02), KFlags(), {'1'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x02 /*&*/), values(KCode(0x02), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2a /*left shift*/), values(KCode(0x2a), KFlags(), {}, KV::Shift));
    RED_CHECK_EQ(keymap.mods().as_uint(), (KeyMod::RShift | KeyMod::LShift).as_uint());
    RED_CHECK_EQ(event(release | 0x36 /*right shift*/), values(KCode(0x36), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(keymap.mods().as_uint(), KeyModFlags(KeyMod::LShift).as_uint());
    RED_CHECK_EQ(event(release | 0x2a /*left shift*/), values(KCode(0x2a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(keymap.mods().as_uint(), 0);
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x10 /*a*/), values(KCode(0x10), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x02 /*&*/), values(KCode(0x02), KFlags(), {'&'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x02 /*&*/), values(KCode(0x02), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x14b /*left*/), values(KCode(0x100 | 0x4b), KFlags(0x0100), {}, KV::LeftArrow));
    RED_CHECK_EQ(event(release | 0x14b /*left*/), values(KCode(0x100 | 0x4b), KFlags(0x8100), {}, KV::None));
    RED_CHECK_EQ(keymap.mods().as_uint(), 0);
    RED_CHECK_EQ(event(downnnn | 0x01 /*esc*/), values(KCode(0x01), KFlags(), {0x1b}, KV::Esc));
    RED_CHECK_EQ(event(release | 0x01 /*esc*/), values(KCode(0x01), KFlags(0x8000), {}, KV::None));

    // cut / copy / paste
    RED_CHECK_EQ(event(downnnn | 0x2d /*x*/), values(KCode(0x2d), KFlags(), {'x'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x2d /*x*/), values(KCode(0x2d), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2e /*c*/), values(KCode(0x2e), KFlags(), {'c'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x2e /*c*/), values(KCode(0x2e), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2f /*v*/), values(KCode(0x2f), KFlags(), {'v'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x2f /*v*/), values(KCode(0x2f), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x1d /*left ctrl*/), values(KCode(0x1d), KFlags(), {}, KV::Ctrl));
    RED_CHECK_EQ(event(downnnn | 0x2d /*x*/), values(KCode(0x2d), KFlags(), {}, KV::Cut));
    RED_CHECK_EQ(event(release | 0x2d /*x*/), values(KCode(0x2d), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2e /*c*/), values(KCode(0x2e), KFlags(), {}, KV::Copy));
    RED_CHECK_EQ(event(release | 0x2e /*c*/), values(KCode(0x2e), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2f /*v*/), values(KCode(0x2f), KFlags(), {}, KV::Paste));
    RED_CHECK_EQ(event(release | 0x2f /*v*/), values(KCode(0x2f), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x1d /*left ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));


    // dead keys
    RED_CHECK_EQ(event(downnnn | 0x1a /*^*/), values(KCode(0x1a), KFlags(), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1a /*^*/), values(KCode(0x1a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x1a /*^*/), values(KCode(0x1a), KFlags(), {'^', '^'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1a /*^*/), values(KCode(0x1a), KFlags(0x8000), {}, KV::None));

    RED_CHECK_EQ(event(downnnn | 0x1a /*^*/), values(KCode(0x1a), KFlags(), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1a /*^*/), values(KCode(0x1a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x22 /*g*/), values(KCode(0x22), KFlags(), {'^', 'g'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x22 /*g*/), values(KCode(0x22), KFlags(0x8000), {}, KV::None));

    RED_CHECK_EQ(event(downnnn | 0x1a /*^*/), values(KCode(0x1a), KFlags(), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1a /*^*/), values(KCode(0x1a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x12 /*e*/), values(KCode(0x12), KFlags(), {0xEA /*ê*/}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x12 /*e*/), values(KCode(0x12), KFlags(0x8000), {}, KV::None));

    RED_CHECK_EQ(event(downnnn | 0x1a /*^*/), values(KCode(0x1a), KFlags(), {}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x1a /*^*/), values(KCode(0x1a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x0E /*backspace*/), values(KCode(0x0E), KFlags(), {}, KV::Backspace));
    RED_CHECK_EQ(event(release | 0x0E /*backspace*/), values(KCode(0x0E), KFlags(0x8000), {}, KV::None));


    // caps lock
    RED_CHECK_EQ(event(downnnn | 0x3a /* capslock */), values(KCode(0x3a), KFlags(), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'A'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x3a /* capslock */), values(KCode(0x3a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'A'}, KV::KeyDown));
    RED_CHECK_EQ(event(downnnn | 0x3a /* capslock */), values(KCode(0x3a), KFlags(), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x3a /* capslock */), values(KCode(0x3a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x10 /*a*/), values(KCode(0x10), KFlags(), {'a'}, KV::KeyDown));

    RED_CHECK_EQ(event(downnnn | 0x56 /*<*/), values(KCode(0x56), KFlags(), {'<'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x56 /*<*/), values(KCode(0x56), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x3a /* capslock */), values(KCode(0x3a), KFlags(), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x3a /* capslock */), values(KCode(0x3a), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x56 /*<*/), values(KCode(0x56), KFlags(), {'<'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x56 /*<*/), values(KCode(0x56), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x3a /* capslock */), values(KCode(0x3a), KFlags(), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x3a /* capslock */), values(KCode(0x3a), KFlags(0x8000), {}, KV::None));


    // key action
    RED_CHECK_EQ(event(downnnn | 0x0E /*backspace*/), values(KCode(0x0E), KFlags(), {'\b'}, KV::Backspace));
    RED_CHECK_EQ(event(release | 0x0E /*backspace*/), values(KCode(0x0E), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x148 /*up*/), values(KCode(0x100 | 0x48), KFlags(0x0100), {}, KV::UpArrow));
    RED_CHECK_EQ(event(release | 0x148 /*up*/), values(KCode(0x100 | 0x48), KFlags(0x8100), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {'\t'}, KV::Tab));
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x2a /*left shift*/), values(KCode(0x2a), KFlags(), {}, KV::Shift));
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {'\t'}, KV::BackTab));
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x2a /*left shift*/), values(KCode(0x2a), KFlags(0x8000), {}, KV::None));


    // numpad
    RED_CHECK_EQ(event(downnnn | 0x4b /*numpad4*/), values(KCode(0x4b), KFlags(), {}, KV::LeftArrow));
    RED_CHECK_EQ(event(release | 0x4b /*numpad4*/), values(KCode(0x4b), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x53 /*numpad.*/), values(KCode(0x53), KFlags(), {}, KV::Delete));
    RED_CHECK_EQ(event(release | 0x53 /*numpad.*/), values(KCode(0x53), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x45 /*verrnum*/), values(KCode(0x45), KFlags(), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x45 /*verrnum*/), values(KCode(0x45), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x4b /*numpad4*/), values(KCode(0x4b), KFlags(), {'4'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x4b /*numpad4*/), values(KCode(0x4b), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x53 /*numpad.*/), values(KCode(0x53), KFlags(), {'.'}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x53 /*numpad.*/), values(KCode(0x53), KFlags(0x8000), {}, KV::None));


    // altgr+e = é
    RED_CHECK_EQ(event(downnnn | 0x138 /*altgr*/), values(KCode(0x138), KFlags(0x100), {}, KV::None));
    RED_CHECK_EQ(event(downnnn | 0x12 /*e*/), values(KCode(0x12), KFlags(), {0x20AC}, KV::KeyDown));
    RED_CHECK_EQ(event(release | 0x12 /*e*/), values(KCode(0x12), KFlags(0x8000), {}, KV::None));
    RED_CHECK_EQ(event(release | 0x138 /*altgr*/), values(KCode(0x138), KFlags(0x8100), {}, KV::None));


    // shortcut for task manager
    // ctrl+alt+del
    RED_CHECK_EQ(event(downnnn | 0x11d /*right ctrl*/), values(KCode(0x100 | 0x1d), KFlags(0x0100), {}, KV::Ctrl));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x38 /*alt*/), values(KCode(0x38), KFlags(), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x153 /*del*/), values(KCode(0x100 | 0x53), KFlags(0x0100), {}, KV::Delete));
    RED_CHECK(keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x38 /*alt*/), values(KCode(0x38), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x11d /*right ctrl*/), values(KCode(0x100 | 0x1d), KFlags(0x8100), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x153 /*del*/), values(KCode(0x100 | 0x53), KFlags(0x8100), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());

    // shortcut for task manager
    // ctrl+shift+esc
    RED_CHECK_EQ(event(downnnn | 0x1d /*ctrl*/), values(KCode(0x1d), KFlags(), {}, KV::Ctrl));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x2a /*shift*/), values(KCode(0x2a), KFlags(), {}, KV::Shift));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x01 /*esc*/), values(KCode(0x01), KFlags(), {}, KV::Esc));
    RED_CHECK(keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x2a /*shift*/), values(KCode(0x2a), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x1d /*ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(release | 0x01 /*esc*/), values(KCode(0x01), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());

    // application switching
    // alt+tab
    RED_CHECK_EQ(event(downnnn | 0x38 /*alt*/), values(KCode(0x38), KFlags(), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {}, KV::Tab));
    RED_CHECK(keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {}, KV::Tab));
    RED_CHECK(keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x38 /*alt*/), values(KCode(0x38), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());

    // application switching
    // alt+ctrl
    RED_CHECK_EQ(event(downnnn | 0x1d /*ctrl*/), values(KCode(0x1d), KFlags(), {}, KV::Ctrl));
    RED_CHECK(!keymap.is_tsk_switch_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {}, KV::Tab));
    RED_CHECK(keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(downnnn | 0x0F /*tab*/), values(KCode(0x0F), KFlags(), {}, KV::Tab));
    RED_CHECK(keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x0F /*tab*/), values(KCode(0x0F), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());
    RED_CHECK_EQ(event(release | 0x1d /*ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));
    RED_CHECK(!keymap.is_app_switching_shortcut());

    // locks
    keymap.set_locks(KeyLocks::NoLocks);
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());
    keymap.set_locks(KeyLocks::CapsLock);
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags(KeyMod::CapsLock).as_uint());
    keymap.set_locks(KeyLocks::CapsLock | KeyLocks::NumLock);
    RED_CHECK(keymap.mods().as_uint() == (KeyMod::CapsLock | KeyMod::NumLock).as_uint());
    keymap.set_locks(KeyLocks::NoLocks);
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());

    // release ctrl without down
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());
    RED_CHECK_EQ(event(release | 0x1d /*left ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());
    RED_CHECK_EQ(event(release | 0x1d /*left ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());
    RED_CHECK_EQ(event(release | 0x1d /*left ctrl*/), values(KCode(0x1d), KFlags(0x8000), {}, KV::None));
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags().as_uint());


    // Test on large codepoint (>= 0x3fff) and Kana

    // Jap layout
    keymap.set_layout(*find_layout_by_id(KeyLayout::KbdId(0x0411)));

    RED_CHECK(keymap.mods().as_uint() == 0);
    RED_CHECK(event(downnnn | 0x10 /*q*/) == values(KCode(0x10), KFlags(), {'q'}, KV::KeyDown));
    RED_CHECK(event(release | 0x10 /*q*/) == values(KCode(0x10), KFlags(0x8000), {}, KV::None));

    // force KanaLock
    keymap.set_locks(KeyLocks::KanaLock);
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags(KeyLocks::KanaLock).as_uint());
    // large codepoint
    RED_CHECK(event(downnnn | 0x10 /*q*/) == values(KCode(0x10), KFlags(), {0xff80}, KV::KeyDown));
    RED_CHECK(event(release | 0x10 /*q*/) == values(KCode(0x10), KFlags(0x8000), {}, KV::None));

    keymap.set_locks(KeyLocks());
    RED_CHECK(keymap.mods().as_uint() == 0);
    // set KanaLock
    RED_CHECK(event(downnnn | 0x72 /* KanaLock */) == values(KCode(0x72), KFlags(), {}, KV::KeyDown));
    RED_CHECK(event(release | 0x72 /* KanaLock */) == values(KCode(0x72), KFlags(0x8000), {}, KV::None));
    RED_CHECK(keymap.mods().as_uint() == KeyModFlags(KeyLocks::KanaLock).as_uint());

    // large codepoint
    RED_CHECK(event(downnnn | 0x10 /*q*/) == values(KCode(0x10), KFlags(), {0xff80}, KV::KeyDown));
    RED_CHECK(event(release | 0x10 /*q*/) == values(KCode(0x10), KFlags(0x8000), {}, KV::None));


    // Test on double dkey

    // German Extended (E2) layout
    keymap.set_layout(*find_layout_by_id(KeyLayout::KbdId(0x30407)));
    keymap.set_locks(KeyLocks());

    // ´ + ^ + a = ấ

    RED_CHECK(event(downnnn | 0x0D /*´*/) == values(KCode(0x0D), KFlags(), {}, KV::KeyDown));
    RED_CHECK(event(release | 0x0D /*´*/) == values(KCode(0x0D), KFlags(0x8000), {}, KV::None));
    RED_CHECK(event(downnnn | 0x29 /*^*/) == values(KCode(0x29), KFlags(), {}, KV::KeyDown));
    RED_CHECK(event(release | 0x29 /*^*/) == values(KCode(0x29), KFlags(0x8000), {}, KV::None));
    RED_CHECK(event(downnnn | 0x1E /*a*/) == values(KCode(0x1E), KFlags(), {0x1EA5/*ấ*/}, KV::KeyDown));
    RED_CHECK(event(release | 0x1E /*a*/) == values(KCode(0x1E), KFlags(0x8000), {}, KV::None));

    // ^ + ´ + a = ấ

    RED_CHECK(event(downnnn | 0x29 /*^*/) == values(KCode(0x29), KFlags(), {}, KV::KeyDown));
    RED_CHECK(event(release | 0x29 /*^*/) == values(KCode(0x29), KFlags(0x8000), {}, KV::None));
    RED_CHECK(event(downnnn | 0x0D /*´*/) == values(KCode(0x0D), KFlags(), {}, KV::KeyDown));
    RED_CHECK(event(release | 0x0D /*´*/) == values(KCode(0x0D), KFlags(0x8000), {}, KV::None));
    RED_CHECK(event(downnnn | 0x1E /*a*/) == values(KCode(0x1E), KFlags(), {0x1EA5/*ấ*/}, KV::KeyDown));
    RED_CHECK(event(release | 0x1E /*a*/) == values(KCode(0x1E), KFlags(0x8000), {}, KV::None));
}
