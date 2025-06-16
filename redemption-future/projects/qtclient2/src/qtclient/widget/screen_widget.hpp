/*
SPDX-FileCopyrightText: 2023 Wallix Proxies Team

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "qtclient/graphics/graphics.hpp"
#include "qtclient/graphics/cursor_cache.hpp"

#include <QtWidgets/QWidget>


class QPixmap;
class RdpInput;

namespace qtclient
{

class ScreenWidget : public QWidget, public qtclient::Graphics
{
public:
    ScreenWidget(QPoint target = QPoint(0, 0));

    void set_rdp_input(RdpInput& rdp_input) noexcept
    {
        _rdp_input = &rdp_input;
    }

    void set_target(QPoint target) noexcept
    {
        _target = target;
    }

    void resizeEvent(QResizeEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

    void new_pointer(gdi::CachePointerIndex cache_idx, RdpPointerView const& pointer) override;

    void cached_pointer(gdi::CachePointerIndex cache_idx) override;

    void sync() override
    {
        update();
    }

    using QWidget::resize;

private:
    RdpInput* _rdp_input = nullptr;
    QPoint _target{0, 0};

    CursorCache _cursor_cache;
    QCursor _shaped_cursor;
};

}
