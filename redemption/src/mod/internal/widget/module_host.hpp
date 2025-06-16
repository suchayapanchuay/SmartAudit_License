/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2016
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#pragma once

#include "core/RDP/gcc/userdata/cs_monitor.hpp"
#include "mod/internal/widget/scroll.hpp"
#include "mod/internal/widget/composite.hpp"
#include "gdi/graphic_api.hpp"
#include "utils/sugar/not_null_ptr.hpp"
#include "utils/ref.hpp"

class mod_api;

class WidgetModuleHost : public WidgetComposite, public gdi::GraphicApi
{
public:
    WidgetModuleHost(
        gdi::GraphicApi& drawable, Widget& screen,
        Ref<mod_api> managed_mod, Font const & font,
        const GCC::UserData::CSMonitor& cs_monitor,
        Rect widget_rect, uint16_t front_width, uint16_t front_height);

    void set_mod(Ref<mod_api> managed_mod) noexcept;

    void draw(RDP::FrameMarker    const & cmd) override;
    void draw(RDPDstBlt           const & cmd, Rect clip) override;
    void draw(RDPMultiDstBlt      const & cmd, Rect clip) override;
    void draw(RDPPatBlt           const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDP::RDPMultiPatBlt const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPOpaqueRect       const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPMultiOpaqueRect  const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPScrBlt           const & cmd, Rect clip) override;
    void draw(RDP::RDPMultiScrBlt const & cmd, Rect clip) override;
    void draw(RDPLineTo           const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPPolygonSC        const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPPolygonCB        const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPPolyline         const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPEllipseSC        const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPEllipseCB        const & cmd, Rect clip, gdi::ColorCtx color_ctx) override;
    void draw(RDPBitmapData       const & cmd, Bitmap const & bmp) override;
    void draw(RDPMemBlt           const & cmd, Rect clip, Bitmap const & bmp) override;
    void draw(RDPMem3Blt          const & cmd, Rect clip, gdi::ColorCtx color_ctx, Bitmap const & bmp) override;
    void draw(RDPGlyphIndex       const & cmd, Rect clip, gdi::ColorCtx color_ctx, GlyphCache const & gly_cache) override;
    void draw(RDPSetSurfaceCommand const & /*cmd*/) override {}
    void draw(RDPSetSurfaceCommand const & /*cmd*/, RDPSurfaceContent const &/*content*/) override {}

    void draw(const RDP::RAIL::NewOrExistingWindow            & cmd) override;
    void draw(const RDP::RAIL::WindowIcon                     & cmd) override;
    void draw(const RDP::RAIL::CachedIcon                     & cmd) override;
    void draw(const RDP::RAIL::DeletedWindow                  & cmd) override;
    void draw(const RDP::RAIL::NewOrExistingNotificationIcons & cmd) override;
    void draw(const RDP::RAIL::DeletedNotificationIcons       & cmd) override;
    void draw(const RDP::RAIL::ActivelyMonitoredDesktop       & cmd) override;
    void draw(const RDP::RAIL::NonMonitoredDesktop            & cmd) override;

    void draw(RDPColCache   const & cmd) override;
    void draw(RDPBrushCache const & cmd) override;

    void cached_pointer(gdi::CachePointerIndex cache_idx) override;
    void new_pointer(gdi::CachePointerIndex cache_idx, RdpPointerView const& cursor) override;

    mod_api& get_managed_mod()
    {
        return *this->managed_mod;
    }

    [[nodiscard]] const mod_api& get_managed_mod() const
    {
        return *this->managed_mod;
    }

    void update_area_and_draw(int16_t x, int16_t y, uint16_t w, uint16_t h);

    using WidgetComposite::set_wh;

    [[nodiscard]] gdi::CachePointerIndex const* get_cache_pointer_index() const override
    {
        return &this->current_cache_pointer_index;
    }

    // RdpInput

    void rdp_input_invalidate(Rect clip) override;

    void rdp_input_mouse(uint16_t device_flags, uint16_t x, uint16_t y) override;

    void rdp_input_scancode(KbdFlags flags, Scancode scancode, uint32_t event_time, Keymap const& keymap) override;

    void rdp_input_synchronize(KeyLocks locks) override;

private:
    class Impl;

    void update_rects(const Dimension module_dim);

    void screen_copy(Rect old_rect, Rect new_rect);

    not_null_ptr<mod_api> managed_mod;

    gdi::GraphicApi& drawable;
    Widget& screen;

    WidgetScrollBar hscroll;
    WidgetScrollBar vscroll;

    bool hscroll_added = false;
    bool vscroll_added = false;

    Rect mod_visible_rect;

    Rect vision_rect;

    GCC::UserData::CSMonitor monitors;

    GCC::UserData::CSMonitor monitor_one;

    gdi::CachePointerIndex current_cache_pointer_index;

    uint16_t current_pointer_pos_x = 0;
    uint16_t current_pointer_pos_y = 0;
};
