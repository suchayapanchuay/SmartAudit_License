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
*   Copyright (C) Wallix 2010-2015
*   Author(s): Jonathan Poelen
*/

#pragma once

#include "mln/image/image2d.hh"

#include "classification.hh"
#include "extract_bars.hh"


namespace ocr {

namespace aux_ {
    template<class ImageView, class TitlebarColor>
    void image_view_to_image2d_bool(
        ImageView const & input, TitlebarColor const & tcolor,
        mln::image2d<bool> & ima, ppocr::Box const & box)
    {
        ima.realloc(box.height(), box.width());
        const unsigned h = box.height();
        const unsigned w = box.width();
        const unsigned icol = box.x();
        for (unsigned y = 0, iy = box.y(); y < h; ++y, ++iy) {
            for (unsigned x = 0, ix = icol; x < w; ++x, ++ix) {
                ima[{x, y}] = tcolor.threshold_chars(input[{ix, iy}]);
            }
        }
    }
} // namespace aux_

template<class ImageView>
void image_view_to_image2d_bool(
    ImageView const & input, unsigned tid,
    mln::image2d<bool> & ima, ppocr::Box const & box)
{
    dispatch_title_color(tid, [&](auto const& tcolor, bool /*is_win2012*/){
        aux_::image_view_to_image2d_bool(input, tcolor, ima, box);
    });
}

struct ExtractTextClassification
{
    template<class ImageView>
    const classifier_type & extract_text(
        ImageView const & input, unsigned tid,
        ppocr::Box const & box,
        ocr::fonts::LocaleId local_id,
        unsigned font_id = -1) /*NOLINT*/
    {
        image_view_to_image2d_bool(input, tid, this->ima, box);
        return this->classification.classify(this->ima, local_id, font_id);
    }

private:
    ocr::Classification classification;
    mln::image2d<bool> ima;
};

} // namespace ocr
