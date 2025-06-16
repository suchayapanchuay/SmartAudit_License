/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2016
*   Author(s): Jonathan Poelen
*/

#pragma once

#include <string>

#include "utils/rect.hpp"


struct OcrTitle
{
    std::string text;
    Rect        rect;
    bool        is_title_bar;
    unsigned    unrecognized_rate;

    OcrTitle(std::string text_, const Rect rect_, bool is_title_bar_, unsigned unrecognized_rate_)
    : text(std::move(text_))
    , rect(rect_)
    , is_title_bar(is_title_bar_)
    , unrecognized_rate(unrecognized_rate_)
    {}
};
