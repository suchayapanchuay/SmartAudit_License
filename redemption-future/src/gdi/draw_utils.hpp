/*
SPDX-FileCopyrightText: 2024 Wallix Proxies Team
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "gdi/graphic_api.hpp"


void gdi_draw_border(
    gdi::GraphicApi& drawable, RDPColor color,
    int16_t x, int16_t y, uint16_t cx, uint16_t cy,
    uint16_t border_width, Rect clip, gdi::ColorCtx color_ctx
);

inline void gdi_draw_border(
    gdi::GraphicApi& drawable, RDPColor color,
    Rect rect, uint16_t border_width, Rect clip, gdi::ColorCtx color_ctx
)
{
    gdi_draw_border(drawable, color, rect.x, rect.y, rect.cx, rect.cy, border_width, clip, color_ctx);
}
