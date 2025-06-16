/*
SPDX-FileCopyrightText: 2024 Wallix Proxies Team
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "utils/sugar/array_view.hpp"

#include <string_view>


inline std::string_view to_sv(chars_view str)
{
    return str.as<std::string_view>();
}
