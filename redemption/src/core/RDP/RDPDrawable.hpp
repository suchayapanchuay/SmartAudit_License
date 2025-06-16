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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
              Martin Potier, Poelen Jonathan, Raphael Zhou, Meng Tan
*/

#pragma once

#include "core/RDP/capabilities/cap_glyphcache.hpp"

#include "gdi/graphic_api.hpp"
#include "gdi/resize_api.hpp"

#include "utils/drawable.hpp"


class RDPDrawable final
: public gdi::GraphicApi, public gdi::ResizeApi
{
    using Color = Drawable::Color;

    Drawable drawable;

    int frame_start_count;
    BGRPalette mod_palette_rgb;

    uint8_t fragment_cache[MAXIMUM_NUMBER_OF_FRAGMENT_CACHE_ENTRIES][1 /* size */ + MAXIMUM_SIZE_OF_FRAGMENT_CACHE_ENTRIE];

public:
    RDPDrawable(const uint16_t width, const uint16_t height);

    void resize(uint16_t width, uint16_t height) override;

    uint8_t * first_pixel() noexcept
    {
        return this->drawable.first_pixel();
    }

    [[nodiscard]] const uint8_t * data() const noexcept
    {
        return this->drawable.data();
    }

    [[nodiscard]] uint16_t width() const noexcept
    {
        return this->drawable.width();
    }

    [[nodiscard]] uint16_t height() const noexcept
    {
        return this->drawable.height();
    }

    static constexpr uint8_t bpp() noexcept
    {
        return Drawable::bpp();
    }

    [[nodiscard]] unsigned size() const noexcept
    {
        return this->drawable.size();
    }

    [[nodiscard]] size_t rowsize() const noexcept
    {
        return this->drawable.rowsize();
    }

    [[nodiscard]] size_t pix_len() const noexcept
    {
        return this->drawable.pix_len();
    }

    // TODO FIXME temporary
    //@{
    Drawable & impl() noexcept
    {
        return this->drawable;
    }

    [[nodiscard]] const Drawable & impl() const noexcept
    {
        return this->drawable;
    }
    //@}

    void set_row(size_t rownum, bytes_view  data) override
    {
        this->drawable.set_row(rownum, data);
    }

    void draw(RDPColCache   const & /*cmd*/) override;
    void draw(RDPBrushCache const & /*cmd*/) override;
    void draw(RDPOpaqueRect const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPEllipseSC const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPEllipseCB const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(const RDPScrBlt & cmd, Rect clip) override;
    void draw(const RDPDstBlt & cmd, Rect clip) override;
    void draw(const RDPMultiDstBlt & cmd, Rect clip) override;
    void draw(RDPMultiOpaqueRect const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDP::RDPMultiPatBlt const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(const RDP::RDPMultiScrBlt & cmd, Rect clip) override;
    void draw(RDPPatBlt const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(const RDPMemBlt & cmd_, Rect clip, const Bitmap & bmp) override;
    void draw(RDPMem3Blt const & cmd, Rect clip, gdi::ColorCtx color_ctx, const Bitmap & bmp) override;
    void draw(RDPSetSurfaceCommand const & cmd) override;
    void draw(RDPSetSurfaceCommand const & cmd, RDPSurfaceContent const &content) override;


    /*
     *
     *            +----+----+
     *            |\   |   /|  4 cases.
     *            | \  |  / |  > Case 1 is the normal case
     *            |  \ | /  |  > Case 2 has a negative coeff
     *            | 3 \|/ 2 |  > Case 3 and 4 are the same as
     *            +----0---->x    Case 1 and 2 but one needs to
     *            | 4 /|\ 1 |     exchange begin and end.
     *            |  / | \  |
     *            | /  |  \ |
     *            |/   |   \|
     *            +----v----+
     *                 y
     *  Anyway, we base the line drawing on bresenham's algorithm
     */
    void draw(const RDPLineTo & lineto, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPGlyphIndex const & cmd, Rect clip, gdi::ColorCtx color_ctx, const GlyphCache & gly_cache) override;
    void draw(RDPPolyline const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPPolygonSC const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPPolygonCB const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(const RDPBitmapData & bitmap_data, const Bitmap & bmp) override;
    void draw(const RDP::FrameMarker & order) override;

    bool logical_frame_ended() const
    {
        return this->drawable.logical_frame_ended;
    }

    void trace_mouse();
    void clear_mouse();

    void draw(const RDP::RAIL::NewOrExistingWindow            & /*unused*/) override {}
    void draw(const RDP::RAIL::WindowIcon                     & /*unused*/) override {}
    void draw(const RDP::RAIL::CachedIcon                     & /*unused*/) override {}
    void draw(const RDP::RAIL::DeletedWindow                  & /*unused*/) override {}
    void draw(const RDP::RAIL::NewOrExistingNotificationIcons & /*unused*/) override {}
    void draw(const RDP::RAIL::DeletedNotificationIcons       & /*unused*/) override {}
    void draw(const RDP::RAIL::ActivelyMonitoredDesktop       & /*unused*/) override {}
    void draw(const RDP::RAIL::NonMonitoredDesktop            & /*unused*/) override {}

    void new_pointer(gdi::CachePointerIndex cache_idx, const RdpPointerView & cursor) override;
    void cached_pointer(gdi::CachePointerIndex cache_idx) override;

    void set_palette(const BGRPalette & palette) override
    {
        this->mod_palette_rgb = palette;
    }

    operator ImageView () const noexcept
    {
        return gdi::get_image_view(this->drawable);
    }
};

namespace gdi
{

    inline ImageView get_image_view(RDPDrawable const & drawable) noexcept
    {
        return gdi::get_image_view(drawable.impl());
    }

    inline WritableImageView get_writable_image_view(RDPDrawable & drawable) noexcept
    {
        return gdi::get_writable_image_view(drawable.impl());
    }
} // namespace gdi
