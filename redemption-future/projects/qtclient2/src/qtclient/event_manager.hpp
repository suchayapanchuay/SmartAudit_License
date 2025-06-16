/*
SPDX-FileCopyrightText: 2023 Wallix Proxies Team

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "core/events.hpp"
#include "core/error.hpp"
#include "utils/basic_function.hpp"

#include <QtCore/QTimer>
#include <QtCore/QSocketNotifier>

#include <memory>


namespace qtclient
{

// replace to QAbstractEventDispatcher ? (https://github.com/svalaskevicius/qt-event-dispatcher-libuv)
struct EventManager
{
    EventManager();

    void start()
    {
        update();
    }

    MonotonicTimePoint const& monotonic_time() const
    {
        return event_container.get_time_base().monotonic_time;
    }

    TimeBase const& update_times();

    EventContainer event_container;
    BasicFunction<void(Error&)> exception_notifier = NullFunction();

private:
    struct FdEvent
    {
        QSocketNotifier notifier;
        Event& event;
    };

    REDEMPTION_NOINLINE
    void add_fdevent(Event* const& event);

    REDEMPTION_NOINLINE
    void remove_fdevent(Event* const& event);

    void execute_fd(FdEvent& fdevent);

    void execute_timer();

    void update();

    QTimer timer;
    std::vector<std::unique_ptr<FdEvent>> fdevents;
};

}
