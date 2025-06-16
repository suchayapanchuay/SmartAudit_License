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

#pragma once

#include "keyboard/keylayout.hpp"

#include <cassert>


array_view<KeyLayout> keylayouts() noexcept;
array_view<KeyLayout> keylayouts_sorted_by_name() noexcept;
KeyLayout const* find_layout_by_id(KeyLayout::KbdId id) noexcept;
KeyLayout const* find_layout_by_name(chars_view name) noexcept;

inline KeyLayout const& default_layout() noexcept
{
  auto* layout = find_layout_by_id(KeyLayout::KbdId(0x409));
  assert(layout);
  return *layout;
}
