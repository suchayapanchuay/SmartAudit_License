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
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Dominique Lafages, Raphael Zhou,
              Meng Tan

   header file. Keymap2 object, used to manage key stroke events
*/

#pragma once

#include "keyboard/keylayout.hpp"
#include "keyboard/kbdtypes.hpp"
#include "keyboard/key_mod_flags.hpp"
#include "utils/sugar/bounded_array_view.hpp"


struct Keymap
{
    using KbdFlags = kbdtypes::KbdFlags;
    using Scancode = kbdtypes::Scancode;
    using KeyCode = kbdtypes::KeyCode;
    using KeyLocks = kbdtypes::KeyLocks;

    using KeyModFlags = kbdtypes::KeyModFlags;

    using unicode16_t = KeyLayout::unicode16_t;

    enum class KEvent : uint8_t
    {
        None,
        KeyDown,
        F4,
        F12,
        Tab,
        BackTab,
        Enter,
        Esc,
        Delete,
        Backspace,
        LeftArrow,
        RightArrow,
        UpArrow,
        DownArrow,
        Home,
        End,
        PgUp,
        PgDown,
        Insert,
        Cut,
        Copy,
        Paste,
        Ctrl,
        Shift,
    };

    struct DecodedKeys
    {
        KeyCode keycode;
        KbdFlags flags;
        // 2 unicode chars when bad dead key
        std::array<uint32_t, 2> uchars;

        bool has_char() const noexcept
        {
            return uchars[0];
        }

        using uchars_view = bounded_array_view<uint32_t, 0, 2>;

        uchars_view uchars_av() const&
        {
            auto len = uchars[0] ? (uchars[1] ? 2u : 1u) : 0u;
            return uchars_view::assumed(uchars.data(), len);
        }
    };


    explicit Keymap(KeyLayout layout) noexcept;

    DecodedKeys event(uint16_t scancode_and_flags) noexcept;
    DecodedKeys event(KbdFlags flags, Scancode scancode) noexcept;

    void reset_decoded_keys() noexcept
    {
        _decoded_key = {};
    }

    DecodedKeys const & last_decoded_keys() const noexcept
    {
        return _decoded_key;
    }

    KeyLayout::DKeyTable current_dkeys_table() const noexcept
    {
        return _dkeys;
    }

    KEvent last_kevent() const noexcept;

    bool is_tsk_switch_shortcut() const noexcept;
    bool is_app_switching_shortcut() const noexcept;

    bool is_session_sharing_take_control() const noexcept;
    bool is_session_sharing_give_control() const noexcept;
    bool is_session_sharing_kill_guest() const noexcept;
    bool is_session_sharing_toggle_graphics() const noexcept;

    bool is_alt_pressed() const noexcept;
    bool is_ctrl_pressed() const noexcept;
    bool is_shift_pressed() const noexcept;

    bool is_session_scuttling_shortcut_pressed() const noexcept;

    void reset_mods(KeyLocks locks) noexcept;

    void set_locks(KeyLocks locks) noexcept;

    void set_layout(KeyLayout new_layout) noexcept;

    KeyLayout const& layout() const noexcept
    {
        return _layout;
    }

    KeyLocks locks() const noexcept;

    KeyModFlags mods() const noexcept;

private:
    void _update_keymap() noexcept;

    DecodedKeys _decoded_key {};
    KeyModFlags _key_mods {};

    std::array<KeyLayout::HalfKeymap, 2> _keymap;
    uint8_t _imods {};
    KeyLayout::DKeyTable _dkeys {};

    KeyLayout _layout;
}; // END STRUCT - Keymap2
