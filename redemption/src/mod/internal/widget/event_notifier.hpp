/*
SPDX-FileCopyrightText: 2024 Wallix Proxies Team

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "utils/basic_function.hpp"


struct WidgetEventNotifier : BasicFunction<void()>
{
    using BasicFunction<void()>::BasicFunction;

    explicit WidgetEventNotifier() noexcept
      : BasicFunction<void()>(NullFunction())
    {}

    explicit WidgetEventNotifier(decltype(nullptr)) noexcept
      : BasicFunction<void()>(NullFunction())
    {}
};
