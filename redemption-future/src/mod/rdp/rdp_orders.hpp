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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean

   rdp module process orders
*/


#pragma once

#include <cstring>
#include <string>

#include <cinttypes>

#include "utils/log.hpp"
#include "utils/stream.hpp"
#include "utils/basic_function.hpp"

#include "core/RDP/protocol.hpp"

#include "core/RDP/orders/RDPOrdersCommon.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryScrBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryDstBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMultiPatBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMultiScrBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryPatBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryLineTo.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryGlyphIndex.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryPolyline.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryEllipseSC.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryBmpCache.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryColorCache.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryFrameMarker.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMem3Blt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMultiDstBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMultiOpaqueRect.hpp"
#include "core/RDP/orders/AlternateSecondaryWindowing.hpp"

#include "core/RDP/caches/bmpcache.hpp"

#ifndef __EMSCRIPTEN__
# include "core/app_path.hpp"
# include "core/RDP/caches/bmpcachepersister.hpp"
# include "transport/file_transport.hpp"
# include "utils/fileutils.hpp"
#endif

#include "core/RDP/caches/glyphcache.hpp"

#include "mod/rdp/rdp_verbose.hpp"
#include "mod/rdp/channels/sespro_api.hpp"

#include "gdi/graphic_api.hpp"

/* orders */
class rdp_orders
{
    // State
    RDPOrderCommon     common;
    RDPMemBlt          memblt;
    RDPMem3Blt         mem3blt;
    RDPOpaqueRect      opaquerect;
    RDPScrBlt          scrblt;
    RDPDstBlt         destblt;
    RDPMultiDstBlt     multidstblt;
    RDPMultiOpaqueRect multiopaquerect;
    RDP::RDPMultiPatBlt multipatblt;
    RDP::RDPMultiScrBlt multiscrblt;
    RDPPatBlt          patblt;
    RDPLineTo          lineto;
    RDPGlyphIndex      glyph_index;
    RDPPolyline        polyline;
    RDPEllipseSC       ellipseSC;

private:
    BitsPerPixel bpp {};

public:
    BGRPalette global_palette;

#ifndef __EMSCRIPTEN__
    std::unique_ptr<BmpCache> bmp_cache;
#endif

private:
    GlyphCache gly_cache;

    const RDPVerbose verbose;

public:
    size_t recv_bmp_cache_count = 0;
    size_t recv_order_count = 0;

private:
    std::string target_host;
    bool silent_reject_windowing_orders;

#ifndef __EMSCRIPTEN__
    bool        enable_persistent_disk_bitmap_cache;
    bool        persist_bitmap_cache_on_disk;
    FileTransport::ErrorNotifier notify_error;
#endif

    sespro_api& sespro;

public:
    rdp_orders( const char * target_host, bool enable_persistent_disk_bitmap_cache
        , bool persist_bitmap_cache_on_disk, bool silent_reject_windowing_orders
        , RDPVerbose verbose, BasicFunction<void(Error const&)> notify_error
        , sespro_api& sespro)
    : common(RDP::PATBLT, Rect(0, 0, 1, 1))
    , memblt(0, Rect(), 0, 0, 0, 0)
    , mem3blt(0, Rect(), 0, 0, 0, RDPColor{}, RDPColor{}, RDPBrush(), 0)
    , opaquerect(Rect(), RDPColor{})
    , scrblt(Rect(), 0, 0, 0)
    , destblt(Rect(), 0)
    , patblt(Rect(), 0, RDPColor{}, RDPColor{}, RDPBrush())
    , lineto(0, 0, 0, 0, 0, RDPColor{}, 0, RDPPen(0, 0, RDPColor{}))
    , glyph_index( 0, 0, 0, 0, RDPColor{}, RDPColor{}, Rect(0, 0, 1, 1), Rect(0, 0, 1, 1), RDPBrush(), 0, 0, 0
                 , byte_ptr_cast(""))
    , global_palette(BGRPalette::classic_332())
    , verbose(verbose)
    , target_host(target_host)
    , silent_reject_windowing_orders(silent_reject_windowing_orders)
#ifndef __EMSCRIPTEN__
    , enable_persistent_disk_bitmap_cache(enable_persistent_disk_bitmap_cache)
    , persist_bitmap_cache_on_disk(persist_bitmap_cache_on_disk)
    , notify_error(std::move(notify_error))
#endif
    , sespro(sespro)
    {
#ifdef __EMSCRIPTEN__
        (void)enable_persistent_disk_bitmap_cache;
        (void)persist_bitmap_cache_on_disk;
        (void)notify_error;
#endif
    }

    void set_bpp(const BitsPerPixel & bpp)
    {
        this->bpp = bpp;
    }

    const BitsPerPixel & get_bpp()
    {
        return this->bpp;
    }

    void reset()
    {
        this->common          = RDPOrderCommon(RDP::PATBLT, Rect(0, 0, 1, 1));
        this->memblt          = RDPMemBlt(0, Rect(), 0, 0, 0, 0);
        this->mem3blt         = RDPMem3Blt(0, Rect(), 0, 0, 0, RDPColor{}, RDPColor{}, RDPBrush(), 0);
        this->opaquerect      = RDPOpaqueRect(Rect(), RDPColor{});
        this->scrblt          = RDPScrBlt(Rect(), 0, 0, 0);
        this->destblt         = RDPDstBlt(Rect(), 0);
        this->multidstblt     = RDPMultiDstBlt();
        this->multiopaquerect = RDPMultiOpaqueRect();
        this->multipatblt     = RDP::RDPMultiPatBlt();
        this->multiscrblt     = RDP::RDPMultiScrBlt();
        this->patblt          = RDPPatBlt(Rect(), 0, RDPColor{}, RDPColor{}, RDPBrush());
        this->lineto          = RDPLineTo(0, 0, 0, 0, 0, RDPColor{}, 0, RDPPen(0, 0, RDPColor{}));
        this->glyph_index     = RDPGlyphIndex( 0, 0, 0, 0, RDPColor{}, RDPColor{}, Rect(0, 0, 1, 1), Rect(0, 0, 1, 1)
                                             , RDPBrush(), 0, 0, 0, byte_ptr_cast(""));
        this->polyline        = RDPPolyline();
    }

#ifndef __EMSCRIPTEN__
    ~rdp_orders() {
        if (this->bmp_cache) {
            try {
                this->save_persistent_disk_bitmap_cache();
            }
            catch(Error const & err) {
                LOG(LOG_ERR, "%s", err.errmsg());
            }
        }
    }

private:
    void save_persistent_disk_bitmap_cache() const
    {
        if (this->persist_bitmap_cache_on_disk)
        {
            ::save_persistent_disk_bitmap_cache(
                *this->bmp_cache,
                app_path(AppPath::PersistentRdp),
                this->target_host.c_str(),
                this->bmp_cache->bpp,
                this->notify_error,
                safe_cast<BmpCachePersister::Verbose>(this->verbose)
            );
        }
    }
#endif

public:
    void create_cache_bitmap(
        gdi::GraphicApi& gd,
        uint16_t small_entries, uint16_t small_size, bool small_persistent,
        uint16_t medium_entries, uint16_t medium_size, bool medium_persistent,
        uint16_t big_entries, uint16_t big_size, bool big_persistent,
        bool enable_waiting_list, BmpCache::Verbose verbose)
    {
#ifdef __EMSCRIPTEN__
        (void)small_size;
        (void)small_persistent;
        (void)medium_size;
        (void)medium_persistent;
        (void)big_size;
        (void)big_persistent;
        (void)verbose;
        gd.set_bmp_cache_entries({
            gdi::GraphicApi::CacheEntry{
                uint16_t(small_entries + (enable_waiting_list ? 1 : 0)),
                small_size,
                small_persistent
            },
            gdi::GraphicApi::CacheEntry{
                uint16_t(medium_entries + (enable_waiting_list ? 1 : 0)),
                medium_size,
                medium_persistent
            },
            gdi::GraphicApi::CacheEntry{
                uint16_t(big_entries + (enable_waiting_list ? 1 : 0)),
                big_size,
                big_persistent
            },
        });
#else
        (void)gd;
        if (this->bmp_cache) {
            if (this->bmp_cache->bpp == this->bpp) {
                return;
            }

            this->save_persistent_disk_bitmap_cache();
            this->bmp_cache.reset();
        }

        this->bmp_cache = std::make_unique<BmpCache>(
            BmpCache::Mod_rdp, this->bpp, 3, false,
            BmpCache::CacheOption(small_entries + (enable_waiting_list ? 1 : 0), small_size, small_persistent),
            BmpCache::CacheOption(medium_entries + (enable_waiting_list ? 1 : 0), medium_size, medium_persistent),
            BmpCache::CacheOption(big_entries + (enable_waiting_list ? 1 : 0), big_size, big_persistent),
            BmpCache::CacheOption(),
            BmpCache::CacheOption(),
            verbose);

        if (this->enable_persistent_disk_bitmap_cache && this->persist_bitmap_cache_on_disk) {
            // Generates the name of file.
            char filename[2048];
            ::snprintf(filename, sizeof(filename) - 1, "%s/PDBC-%s-%d",
                app_path(AppPath::PersistentRdp).c_str(), this->target_host.c_str(),
                underlying_cast(this->bmp_cache->bpp));
            filename[sizeof(filename) - 1] = '\0';

            int fd = ::open(filename, O_RDONLY);
            if (fd == -1) {
                return;
            }

            InFileTransport ift(unique_fd{fd});

            try {
                LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO,
                    "rdp_orders::create_cache_bitmap: filename=\"%s\"", filename);
                const auto cache_persister_verbosity = BmpCachePersister::Verbose()
                    | (bool(this->verbose & RDPVerbose::cache_from_disk)
                         ? BmpCachePersister::Verbose::from_disk
                         : BmpCachePersister::Verbose())
                    | (bool(this->verbose & RDPVerbose::bmp_info)
                         ? BmpCachePersister::Verbose::bmp_info
                         : BmpCachePersister::Verbose())
                    ;
                BmpCachePersister::load_all_from_disk(
                    *this->bmp_cache, ift, filename,
                    cache_persister_verbosity
                );
            }
            catch (...) {
            }
        }
#endif
    }

private:
    void process_framemarker( InStream & stream, const RDP::AltsecDrawingOrderHeader & header
                            , gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders::process_framemarker");

        RDP::FrameMarker order;

        order.receive(stream, header);

        gd.draw(order);
    }

    void process_windowing( InStream & stream, const RDP::AltsecDrawingOrderHeader & header
                          , gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders::process_windowing");

        const uint32_t FieldsPresentFlags = [&]{
            InStream stream2(stream.remaining_bytes());
            stream2.in_skip_bytes(2);    // OrderSize(2)
            return stream2.in_uint32_le();
        }();

        switch (FieldsPresentFlags & ( uint32_t(RDP::RAIL::WINDOW_ORDER_TYPE_WINDOW)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_TYPE_NOTIFY)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_TYPE_DESKTOP)))
        {
            case RDP::RAIL::WINDOW_ORDER_TYPE_WINDOW:
                this->process_window_information(stream, header, FieldsPresentFlags, gd);
                break;

            case RDP::RAIL::WINDOW_ORDER_TYPE_NOTIFY:
                this->process_notification_icon_information(stream, header, FieldsPresentFlags, gd);
                break;

            case RDP::RAIL::WINDOW_ORDER_TYPE_DESKTOP:
                this->process_desktop_information(stream, header, FieldsPresentFlags, gd);
                break;

            default:
                LOG(LOG_INFO,
                    "rdp_orders::process_windowing: "
                        "unsupported Windowing Alternate Secondary Drawing Orders! "
                        "FieldsPresentFlags=0x%08X",
                    FieldsPresentFlags);
                break;
        }
    }

    void process_window_information( InStream & stream, const RDP::AltsecDrawingOrderHeader &
                                    /*unused*/, uint32_t FieldsPresentFlags, gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders::process_window_information");

        switch (FieldsPresentFlags & ( uint32_t(RDP::RAIL::WINDOW_ORDER_STATE_NEW)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_ICON)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_CACHEDICON)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_STATE_DELETED)))
        {
            case RDP::RAIL::WINDOW_ORDER_ICON: {
                    RDP::RAIL::WindowIcon order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                }
                break;

            case RDP::RAIL::WINDOW_ORDER_CACHEDICON: {
                    RDP::RAIL::CachedIcon order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                }
                break;

            case RDP::RAIL::WINDOW_ORDER_STATE_DELETED: {
                    RDP::RAIL::DeletedWindow order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                    this->sespro.rail_remove_window_or_notification_icon(
                          order.header.WindowId()
                        , sespro_api::invalid_notification_icon_id
                        );
                }
                break;

            case 0:
            case RDP::RAIL::WINDOW_ORDER_STATE_NEW: {
                    RDP::RAIL::NewOrExistingWindow order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                    if (order.header.FieldsPresentFlags() & RDP::RAIL::WINDOW_ORDER_FIELD_SHOW)
                    {
                        if (order.ShowState()) {
                            this->sespro.rail_add_window_or_notification_icon(
                                  order.header.WindowId()
                                , sespro_api::invalid_notification_icon_id
                                );
                        }
                        else {
                            this->sespro.rail_remove_window_or_notification_icon(
                                  order.header.WindowId()
                                , sespro_api::invalid_notification_icon_id
                                );
                        }
                    }
                }
                break;

            default:
                LOG(LOG_INFO,
                    "rdp_orders::process_window_information: "
                        "unsupported Windowing Alternate Secondary Drawing Orders! "
                        "FieldsPresentFlags=0x%08X",
                    FieldsPresentFlags);
                break;
        }
    }

    void process_notification_icon_information( InStream & stream, const RDP::AltsecDrawingOrderHeader &
                                               /*unused*/, uint32_t FieldsPresentFlags, gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders::process_notification_icon_information");

        switch (FieldsPresentFlags & ( uint32_t(RDP::RAIL::WINDOW_ORDER_STATE_NEW)
                                     | uint32_t(RDP::RAIL::WINDOW_ORDER_STATE_DELETED)))
        {
            case RDP::RAIL::WINDOW_ORDER_STATE_DELETED: {
                    RDP::RAIL::DeletedNotificationIcons order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                    this->sespro.rail_remove_window_or_notification_icon(
                          order.header.WindowId()
                        , order.header.NotifyIconId()
                        );
                }
                break;

            case 0:
            case RDP::RAIL::WINDOW_ORDER_STATE_NEW: {
                    RDP::RAIL::NewOrExistingNotificationIcons order;
                    order.receive(stream);
                    if (bool(this->verbose & RDPVerbose::rail_order)) {
                        order.log(LOG_INFO);
                    }
                    if (!this->silent_reject_windowing_orders) {
                        gd.draw(order);
                    }
                    this->sespro.rail_add_window_or_notification_icon(
                          order.header.WindowId()
                        , order.header.NotifyIconId()
                        );
                }
                break;

            default:
                LOG(LOG_INFO,
                    "rdp_orders::process_notification_icon_information: "
                        "unsupported Windowing Alternate Secondary Drawing Orders! "
                        "FieldsPresentFlags=0x%08X",
                    FieldsPresentFlags);
                break;
        }
    }

    void process_desktop_information( InStream & stream, const RDP::AltsecDrawingOrderHeader &
                                     /*unused*/, uint32_t FieldsPresentFlags, gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders::process_desktop_information");

        if (FieldsPresentFlags & RDP::RAIL::WINDOW_ORDER_FIELD_DESKTOP_NONE) {
            RDP::RAIL::NonMonitoredDesktop order;
            order.receive(stream);
            if (bool(this->verbose & RDPVerbose::rail_order)) {
                order.log(LOG_INFO);
            }
            if (!this->silent_reject_windowing_orders) {
                gd.draw(order);
            }
        }
        else {
            RDP::RAIL::ActivelyMonitoredDesktop order;
            order.receive(stream);
            if (bool(this->verbose & RDPVerbose::rail_order)) {
                order.log(LOG_INFO);
            }
            if (!this->silent_reject_windowing_orders) {
                gd.draw(order);
            }
        }
    }

    void process_bmpcache(InStream & stream, const RDPSecondaryOrderHeader & header, gdi::GraphicApi& gd)
    {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders_process_bmpcache bpp=%u", this->bpp);
        RDPBmpCache bmp(bool(this->verbose));
        bmp.receive(stream, header, this->global_palette, this->bpp);

        this->recv_bmp_cache_count++;

        assert(bmp.bmp.is_valid());

#ifdef __EMSCRIPTEN__
        gd.draw(bmp);
#else
        (void)gd;
        this->bmp_cache->put(bmp.id, bmp.idx, bmp.bmp, bmp.key1, bmp.key2);
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO,
            "rdp_orders_process_bmpcache bitmap id=%d idx=%d cx=%" PRIu16 " cy=%" PRIu16
            " bmp_size=%zu original_bpp=%" PRIu8 " bpp=%" PRIu8,
            bmp.id, bmp.idx, bmp.bmp.cx(), bmp.bmp.cy(), bmp.bmp.bmp_size(), bmp.bmp.bpp(), this->bpp);
#endif
    }

    void server_add_char( uint8_t cacheId, uint16_t cacheIndex
                        , int16_t offset, int16_t baseline
                        , uint16_t width, uint16_t height, const uint8_t * data)
    {
        RDPFontChar fi(offset, baseline, width, height, 0);
        memcpy(fi.data.get(), data, fi.datasize());

        this->gly_cache.set_glyph(std::move(fi), cacheId, cacheIndex);
    }

    void process_glyphcache(InStream & stream, const RDPSecondaryOrderHeader &/* header*/) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders_process_glyphcache");
        const uint8_t cacheId = stream.in_uint8();
        const uint8_t nglyphs = stream.in_uint8();
        for (uint8_t i = 0; i < nglyphs; i++) {
            const uint16_t cacheIndex = stream.in_uint16_le();
            const int16_t  offset     = stream.in_sint16_le();
            const int16_t  baseline   = stream.in_sint16_le();
            const uint16_t width      = stream.in_uint16_le();
            const uint16_t height     = stream.in_uint16_le();

            const unsigned int   datasize = (height * nbbytes(width) + 3) & ~3;
            const uint8_t      * data     = stream.in_skip_bytes(datasize).data();

            this->server_add_char(cacheId, cacheIndex, offset, baseline, width, height, data);
        }
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "rdp_orders_process_glyphcache done");
    }

    void process_colormap(InStream & stream, const RDPSecondaryOrderHeader & header, gdi::GraphicApi & gd) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "process_colormap");
        RDPColCache colormap;
        colormap.receive(stream, header);
        RDPColCache cmd(colormap.cacheIndex, colormap.palette);
        gd.draw(cmd);

        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "process_colormap done");
    }

public:
    /*****************************************************************************/
    int process_orders(
        InStream & stream, bool fast_path, gdi::GraphicApi & gd,
        uint16_t front_width, uint16_t front_height
    ) {
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "process_orders bpp=%u", this->bpp);

        using namespace RDP;

        OrdersUpdate_Recv orders_update(stream, fast_path);

        this->recv_order_count += orders_update.number_orders;

        int processed = 0;
        while (processed < orders_update.number_orders) {
            DrawingOrder_RecvFactory drawing_order(stream);

            const uint8_t klass = (drawing_order.control_flags & (STANDARD | SECONDARY));
            switch (klass) {
            case SECONDARY: {
                LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "Alternate secondary order");

                RDP::AltsecDrawingOrderHeader header(drawing_order.control_flags);
                REDEMPTION_DIAGNOSTIC_PUSH()
                REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wswitch-enum")
                switch (header.orderType) {
                    case RDP::AltsecDrawingOrderType::SwitchSurface: {
                        uint16_t bitmapId = stream.in_uint16_le();
                        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "switchSurface bitmapId=0x%x", bitmapId);
                        break;
                    }
                    case RDP::AltsecDrawingOrderType::FrameMarker:
                        this->process_framemarker(stream, header, gd);
                    break;
                    case RDP::AltsecDrawingOrderType::Window:
                        this->process_windowing(stream, header, gd);
                    break;
                    default:
                        LOG(LOG_ERR, "unsupported Alternate Secondary Drawing Order (%d)", header.orderType);
                        /* error, unknown order */
                    break;
                }
                REDEMPTION_DIAGNOSTIC_POP()

                break;
            }
            case STANDARD | SECONDARY: {
                //uint8_t * order_start = stream.p;
                RDPSecondaryOrderHeader header(stream);
                //LOG(LOG_INFO, "secondary order=%d", header.type);
                uint8_t const * next_order = stream.get_current() + header.order_data_length();
                switch (header.type) {
                case TS_CACHE_BITMAP_COMPRESSED:
                case TS_CACHE_BITMAP_UNCOMPRESSED:
                case TS_CACHE_BITMAP_COMPRESSED_REV2:
                case TS_CACHE_BITMAP_UNCOMPRESSED_REV2:
                    LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "TS_CACHE_BITMAP XXX secondary order");
                    this->process_bmpcache(stream, header, gd);
                    break;
                case TS_CACHE_COLOR_TABLE:
                    LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "TS_CACHE_COLOR_TABLE secondary order");
                    this->process_colormap(stream, header, gd);
                    break;
                case TS_CACHE_GLYPH:
                    LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "TS_CACHE_GLYPH secondary order");
                    this->process_glyphcache(stream, header);
                    //hexdump_d(order_start, stream.p - order_start);
                    break;
                case TS_CACHE_BITMAP_COMPRESSED_REV3:
                    LOG( LOG_ERR, "unsupported SECONDARY ORDER TS_CACHE_BITMAP_COMPRESSED_REV3 (%u)"
                       , header.type);
                    break;
                default:
                    LOG(LOG_ERR, "unsupported SECONDARY ORDER (%u)", header.type);
                    /* error, unknown order */
                    break;
                }
                stream.in_skip_bytes(next_order - stream.get_current());
                break;
            }
            case STANDARD: {
                RDPPrimaryOrderHeader header = this->common.receive(stream, drawing_order.control_flags);
                const Rect cmd_clip =
                    (drawing_order.control_flags & BOUNDS)
                    ? this->common.clip
                    : Rect(0, 0, front_width, front_height)
                ;
                switch (this->common.order) {
                case GLYPHINDEX:
                    this->glyph_index.receive(stream, header);
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->glyph_index.log(LOG_INFO, cmd_clip);}
                    gd.draw(this->glyph_index, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette), this->gly_cache);
                    break;
                case DESTBLT:
                    this->destblt.receive(stream, header);
                    gd.draw(this->destblt, cmd_clip);
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->destblt.log(LOG_INFO, cmd_clip);}
                    break;
                case MULTIDSTBLT:
                    this->multidstblt.receive(stream, header);
                    gd.draw(this->multidstblt, cmd_clip);
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->multidstblt.log(LOG_INFO, cmd_clip);}
                    break;
                case MULTIOPAQUERECT:
                    this->multiopaquerect.receive(stream, header);
                    gd.draw(this->multiopaquerect, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->multiopaquerect.log(LOG_INFO, cmd_clip);}
                    break;
                case MULTIPATBLT:
                    this->multipatblt.receive(stream, header);
                    gd.draw(this->multipatblt, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->multipatblt.log(LOG_INFO, cmd_clip);}
                    break;
                case MULTISCRBLT:
                    this->multiscrblt.receive(stream, header);
                    gd.draw(this->multiscrblt, cmd_clip);
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->multiscrblt.log(LOG_INFO, cmd_clip);}
                    break;
                case PATBLT:
                    this->patblt.receive(stream, header);
                    gd.draw(this->patblt, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->patblt.log(LOG_INFO, cmd_clip);}
                    break;
                case SCREENBLT:
                    this->scrblt.receive(stream, header);
                    gd.draw(this->scrblt, cmd_clip);
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->scrblt.log(LOG_INFO, cmd_clip);}
                    break;
                case LINE:
                    this->lineto.receive(stream, header);
                    gd.draw(this->lineto, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->lineto.log(LOG_INFO, cmd_clip); }
                    break;
                case RECT:
                    this->opaquerect.receive(stream, header);
                    gd.draw(this->opaquerect, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->opaquerect.log(LOG_INFO, cmd_clip); }
                    break;
                case MEMBLT:
                    this->memblt.receive(stream, header);
                    #ifdef __EMSCRIPTEN__
                    gd.draw(this->memblt, cmd_clip);
                    #else
                    {
                        if ((this->memblt.cache_id >> 8) >= 6) {
                            LOG( LOG_INFO, "colormap out of range in memblt:%x"
                               , unsigned(this->memblt.cache_id >> 8));
                            this->memblt.log(LOG_INFO, cmd_clip);
                            assert(false);
                        }
                        const Bitmap & bitmap =
                            this->bmp_cache->get(this->memblt.cache_id & 0x3, this->memblt.cache_idx);
                        // TODO CGR: check if bitmap has the right palette...
                        // TODO CGR: 8 bits palettes should probabily be transmitted to front, not stored in bitmaps
                        if (bitmap.is_valid()) {
                            gd.draw(this->memblt, cmd_clip, bitmap);
                            if (bool(this->verbose & RDPVerbose::graphics)){ this->memblt.log(LOG_INFO, cmd_clip); }
                        }
                        else {
                            LOG(LOG_ERR, "rdp_orders::process_orders: MEMBLT - Bitmap is not found in cache! cache_id=%u cache_index=%u",
                                this->memblt.cache_id & 0x3u, this->memblt.cache_idx);
                            assert(false);
                        }
                    }
                    #endif
                    break;
                case MEM3BLT:
                    this->mem3blt.receive(stream, header);
                    #ifdef __EMSCRIPTEN__
                    gd.draw(this->mem3blt, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    #else
                    {
                        if ((this->mem3blt.cache_id >> 8) >= 6){
                            LOG( LOG_INFO, "colormap out of range in mem3blt: %x"
                               , unsigned(this->mem3blt.cache_id >> 8));
                            this->mem3blt.log(LOG_INFO, cmd_clip);
                            assert(false);
                        }
                        const Bitmap & bitmap =
                            this->bmp_cache->get(this->mem3blt.cache_id & 0x3, this->mem3blt.cache_idx);
                        // TODO CGR: check if bitmap has the right palette...
                        // TODO CGR: 8 bits palettes should probabily be transmitted to front, not stored in bitmaps
                        if (bitmap.is_valid()) {
                            gd.draw(this->mem3blt, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette), bitmap);
                            if (bool(this->verbose & RDPVerbose::graphics)){ this->mem3blt.log(LOG_INFO, cmd_clip); }
                        }
                        else {
                            LOG(LOG_ERR, "rdp_orders::process_orders: MEM3BLT - Bitmap is not found in cache! cache_id=%u cache_index=%u",
                                this->mem3blt.cache_id & 0x3u, this->mem3blt.cache_idx);
                            assert(false);
                        }
                    }
                    #endif
                    break;
                case POLYLINE:
                    this->polyline.receive(stream, header);
                    gd.draw(this->polyline, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){this->polyline.log(LOG_INFO, cmd_clip); }
                    break;
                case ELLIPSESC:
                    this->ellipseSC.receive(stream, header);
                    gd.draw(this->ellipseSC, cmd_clip, gdi::ColorCtx::from_bpp(this->bpp, this->global_palette));
                    if (bool(this->verbose & RDPVerbose::graphics)){ this->ellipseSC.log(LOG_INFO, cmd_clip); }
                    break;
                default:
                    /* error unknown order */
                    LOG(LOG_ERR, "unsupported PRIMARY ORDER (%d)", this->common.order);
                    break;
                }
                break;
            }
            default: {
                /* error, this should always be set */
                LOG(LOG_ERR, "Unsupported drawing order detected : protocol error. flags=0x%02X class=0x%02X", drawing_order.control_flags, klass);
                assert(false);
                break;
            }
            }
            processed++;
        }
        LOG_IF(bool(this->verbose & RDPVerbose::graphics), LOG_INFO, "process_orders done");
        return 0;
    }   // int process_orders(uint8_t bpp, Stream & stream, bool fast_path, gdi::GraphicApi & gd)
};
