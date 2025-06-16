/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean

*/

#include "test_only/test_framework/redemption_unit_tests.hpp"
// #include "test_only/test_framework/check_img.hpp"

#include "qtclient/graphics/rdp_painter.hpp"
// #include "core/RDP/caches/glyphcache.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryScrBlt.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryGlyphIndex.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryPolyline.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryMultiOpaqueRect.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryMultiDstBlt.hpp"
// #include "core/RDP/orders/RDPOrdersPrimaryMultiScrBlt.hpp"
// #include "utils/bitmap.hpp"
// #include "utils/rect.hpp"
// #include "utils/stream.hpp"
// #include "utils/png.hpp"

// #define IMG_TEST_PATH "../../" FIXTURES_PATH "/img_ref/core/RDP/rdp_drawable/"

RED_AUTO_TEST_CASE(TestRdpPainter)
{
    uint16_t width = 640;
    uint16_t height = 480;
    Rect screen_rect(0, 0, width, height);
    auto const color_cxt = gdi::ColorCtx::depth24();
    QImage img(width, height, QImage::Format_RGB16);
    qtclient::RdpPainter gd(&img);
    gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(NamedBGRColor::YELLOW)), screen_rect, color_cxt);
    gd.draw(RDPOpaqueRect(screen_rect.shrink(5), encode_color24()(NamedBGRColor::GREEN)), screen_rect, color_cxt);
    gd.draw(RDPOpaqueRect(screen_rect.shrink(50), encode_color24()(NamedBGRColor::RED)), screen_rect, color_cxt);

    gd.draw(RDPScrBlt(screen_rect.shrink(100), 0x55, 0, 0), screen_rect);

    img.save("/home/jonathan/rawdisk2/a.png");

    // StaticOutStream<1024> deltaRectangles;
    //
    // out_rect(deltaRectangles, 0, 177, 1263, 530);
    // out_rect(deltaRectangles, 0, -4, 462, 4);
    // out_rect(deltaRectangles, 70, 0, 463, 4);
    //
    // InStream deltaRectangles_in(deltaRectangles.get_produced_bytes());
    //
    // gd.draw(RDP::RDPMultiScrBlt(Rect(0, 173, 1263, 534), 0xCC, 0, -105, 3, deltaRectangles_in), screen_rect);
    //
    // RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_4.png");
}

//
//
// inline void server_add_char(
//     GlyphCache & gly_cache, uint8_t cacheId, uint16_t cacheIndex
//   , int16_t offset, int16_t baseline, uint16_t width, uint16_t height, const uint8_t * data)
// {
//     FontChar fi(offset, baseline, width, height, 0);
//     memcpy(fi.data.get(), data, fi.datasize());
//     // LOG(LOG_INFO, "cacheId=%u cacheIndex=%u", cacheId, cacheIndex);
//     gly_cache.set_glyph(std::move(fi), cacheId, cacheIndex);
// }
//
// inline void process_glyphcache(GlyphCache & gly_cache, InStream & stream) {
//     const uint8_t cacheId = stream.in_uint8();
//     const uint8_t nglyphs = stream.in_uint8();
//     for (uint8_t i = 0; i < nglyphs; i++) {
//         const uint16_t cacheIndex = stream.in_uint16_le();
//         const int16_t  offset     = stream.in_sint16_le();
//         const int16_t  baseline   = stream.in_sint16_le();
//         const uint16_t width      = stream.in_uint16_le();
//         const uint16_t height     = stream.in_uint16_le();
//
//         const unsigned int   datasize = (height * nbbytes(width) + 3) & ~3;
//         const uint8_t      * data     = stream.in_skip_bytes(datasize).data();
//
//         server_add_char(gly_cache, cacheId, cacheIndex, offset, baseline, width, height, data);
//     }
// }
//
// RED_AUTO_TEST_CASE(TestGraphicGlyphIndex)
// {
//     uint16_t width = 1024;
//     uint16_t height = 768;
//     Rect screen_rect(0, 0, width, height);
//     RDPDrawable gd(width, height);
//
//     auto const color_cxt = gdi::ColorCtx::depth24();
//
//     gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(BLACK)), screen_rect, color_cxt);
//
//     GlyphCache gly_cache;
//
//     {
//         uint8_t glyph_cache_data[] = {
// /* 0000 */ 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x07, 0x00, 0x08, 0x00, 0xf8, 0xcc, 0xc6, 0xc6,  // ................
// /* 0010 */ 0xc6, 0xc6, 0xcc, 0xf8, 0x01, 0x00, 0x00, 0x00, 0xf7, 0xff, 0x06, 0x00, 0x09, 0x00, 0x18, 0x30,  // ...............0
// /* 0020 */ 0x00, 0x78, 0xcc, 0xfc, 0xc0, 0xc0, 0x7c, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xfa, 0xff,  // .x....|.........
// /* 0030 */ 0x0a, 0x00, 0x06, 0x00, 0xfb, 0x80, 0xcc, 0xc0, 0xcc, 0xc0, 0xcc, 0xc0, 0xcc, 0xc0, 0xcc, 0xc0,  // ................
// /* 0040 */ 0x03, 0x00, 0x00, 0x00, 0xfa, 0xff, 0x06, 0x00, 0x06, 0x00, 0x78, 0x0c, 0x7c, 0xcc, 0xcc, 0x7c,  // ..........x.|..|
// /* 0050 */ 0x18, 0x00, 0x04, 0x00, 0x00, 0x00, 0xfa, 0xff, 0x04, 0x00, 0x06, 0x00, 0xd0, 0xf0, 0xc0, 0xc0,  // ................
// /* 0060 */ 0xc0, 0xc0, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xfa, 0xff, 0x06, 0x00, 0x06, 0x00, 0x78, 0xcc,  // ..............x.
// /* 0070 */ 0xfc, 0xc0, 0xc0, 0x7c, 0x18, 0x00, 0x09, 0x1b, 0xeb, 0x03, 0x38, 0x07, 0x03, 0x01, 0xd4, 0xd0,  // ...|......8.....
// /* 0080 */ 0xc8, 0x16, 0x00, 0xea, 0x02, 0x4d, 0x00, 0xf7, 0x02, 0x16, 0x00, 0xf5, 0x02, 0x13, 0x00, 0x00,  // .....M..........
// /* 0090 */ 0x01, 0x08, 0x02, 0x07, 0x03, 0x0b, 0x04, 0x07, 0x04, 0x05, 0x05, 0x05, 0x04, 0x07, 0xff, 0x00,  // ................
// /* 00a0 */ 0x10,                                               // .
//         };
//         InStream stream(glyph_cache_data);
//         process_glyphcache(gly_cache, stream);
//
//         Rect rect_bk(22, 746, 56, 14);
//         Rect rect_op(0, 0, 1, 1);
//         RDPBrush brush;
//         RDPGlyphIndex glyph_index(7,                            // cacheId
//                                   3,                            // flAccel
//                                   0,                            // ulCharInc
//                                   1,                            // fOpRedundant
//                                   encode_color24()(BGRColor(0x000000)),           // BackColor
//                                   encode_color24()(BGRColor(0xc8d0d4)),           // ForeColor
//                                   rect_bk,                      // Bk
//                                   rect_op,                      // Op
//                                   RDPBrush(),                   // Brush
//                                   22,                           // X
//                                   757,                          // Y
//                                   19,                           // cbData
//                                   byte_ptr_cast(                // rgbData
//                                           "\x00\x00\x01\x08"
//                                           "\x02\x07\x03\x0b"
//                                           "\x04\x07\x04\x05"
//                                           "\x05\x05\x04\x07"
//                                           "\xff\x00\x10"
//                                       )
//                                  );
//
//         Rect rect_clip(0, 0, 1024, 768);
//         gd.draw(glyph_index, rect_clip, color_cxt, gly_cache);
//
//         RDPGlyphIndex glyph_index_1(7,                              // cacheId
//                                     3,                              // flAccel
//                                     0,                              // ulCharInc
//                                     1,                              // fOpRedundant
//                                     encode_color24()(BGRColor(0x000000)),             // BackColor
//                                     encode_color24()(BGRColor(0xc8d0d4)),             // ForeColor
//                                     rect_bk,                        // Bk
//                                     rect_op,                        // Op
//                                     RDPBrush(),                     // Brush
//                                     22,                             // X
//                                     757,                            // Y
//                                     3,                              // cbData
//                                     byte_ptr_cast("\xfe\x00\x00")   // rgbData
//                                    );
//
//         gd.draw(glyph_index, rect_clip, color_cxt, gly_cache);
//     }
//
//     RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_1.png");
// }
//
// RED_AUTO_TEST_CASE(TestPolyline)
// {
//     // Create a simple capture image and dump it to file
//     uint16_t width = 640;
//     uint16_t height = 480;
//     Rect screen_rect(0, 0, width, height);
//     RDPDrawable gd(width, height);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(WHITE)), screen_rect, color_cxt);
//     gd.draw(RDPOpaqueRect(screen_rect.shrink(5), encode_color24()(BLACK)), screen_rect, color_cxt);
//
//     StaticOutStream<1024> deltaPoints;
//
//     deltaPoints.out_sint16_le(0);
//     deltaPoints.out_sint16_le(20);
//
//     deltaPoints.out_sint16_le(160);
//     deltaPoints.out_sint16_le(0);
//
//     deltaPoints.out_sint16_le(0);
//     deltaPoints.out_sint16_le(-30);
//
//     deltaPoints.out_sint16_le(50);
//     deltaPoints.out_sint16_le(50);
//
//     deltaPoints.out_sint16_le(-50);
//     deltaPoints.out_sint16_le(50);
//
//     deltaPoints.out_sint16_le(0);
//     deltaPoints.out_sint16_le(-30);
//
//     deltaPoints.out_sint16_le(-160);
//     deltaPoints.out_sint16_le(0);
//
//     InStream dp(deltaPoints.get_produced_bytes());
//
//     gd.draw(RDPPolyline(158, 230, 0x06, 0, encode_color24()(WHITE), 7, dp), screen_rect, color_cxt);
//
//     RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_2.png");
// }
//
// namespace
// {
//     void out_rect(OutStream& out_stream, int16_t x, int16_t y, int16_t cx, int16_t cy)
//     {
//         out_stream.out_sint16_le(x);
//         out_stream.out_sint16_le(y);
//         out_stream.out_sint16_le(cx);
//         out_stream.out_sint16_le(cy);
//     }
// }
//
// RED_AUTO_TEST_CASE(TestMultiDstBlt)
// {
//     // Create a simple capture image and dump it to file
//     uint16_t width = 640;
//     uint16_t height = 480;
//     Rect screen_rect(0, 0, width, height);
//     RDPDrawable gd(width, height);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(WHITE)), screen_rect, color_cxt);
//     gd.draw(RDPOpaqueRect(screen_rect.shrink(5), encode_color24()(GREEN)), screen_rect, color_cxt);
//
//     StaticOutStream<1024> deltaRectangles;
//
//     out_rect(deltaRectangles, 100, 100, 10, 10);
//
//     for (int i = 0; i < 19; i++) {
//         out_rect(deltaRectangles, 10, 10, 10, 10);
//     }
//
//     InStream deltaRectangles_in(deltaRectangles.get_produced_bytes());
//
//     gd.draw(RDPMultiDstBlt(100, 100, 200, 200, 0x55, 20, deltaRectangles_in), screen_rect);
//
//     RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_3.png");
// }
//
// RED_AUTO_TEST_CASE(TestMultiScrBlt)
// {
//     // Create a simple capture image and dump it to file
//     uint16_t width = 640;
//     uint16_t height = 480;
//     Rect screen_rect(0, 0, width, height);
//     RDPDrawable gd(width, height);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(WHITE)), screen_rect, color_cxt);
//     gd.draw(RDPOpaqueRect(screen_rect.shrink(5), encode_color24()(GREEN)), screen_rect, color_cxt);
//     gd.draw(RDPOpaqueRect(screen_rect.shrink(50), encode_color24()(RED)), screen_rect, color_cxt);
//
//     StaticOutStream<1024> deltaRectangles;
//
//     out_rect(deltaRectangles, 0, 177, 1263, 530);
//     out_rect(deltaRectangles, 0, -4, 462, 4);
//     out_rect(deltaRectangles, 70, 0, 463, 4);
//
//     InStream deltaRectangles_in(deltaRectangles.get_produced_bytes());
//
//     gd.draw(RDP::RDPMultiScrBlt(Rect(0, 173, 1263, 534), 0xCC, 0, -105, 3, deltaRectangles_in), screen_rect);
//
//     RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_4.png");
// }
//
// RED_AUTO_TEST_CASE(TestMultiOpaqueRect)
// {
//     // Create a simple capture image and dump it to file
//     uint16_t width = 640;
//     uint16_t height = 480;
//     Rect screen_rect(0, 0, width, height);
//     RDPDrawable gd(width, height);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     gd.draw(RDPOpaqueRect(screen_rect, encode_color24()(WHITE)), screen_rect, color_cxt);
//     gd.draw(RDPOpaqueRect(screen_rect.shrink(5), encode_color24()(GREEN)), screen_rect, color_cxt);
//
//     StaticOutStream<1024> deltaRectangles;
//
//     out_rect(deltaRectangles, 100, 100, 10, 10);
//
//     for (int i = 0; i < 19; i++) {
//         out_rect(deltaRectangles, 10, 10, 10, 10);
//     }
//
//     InStream deltaRectangles_in(deltaRectangles.get_produced_bytes());
//
//     gd.draw(RDPMultiOpaqueRect(100, 100, 200, 200, encode_color24()(BLACK), 20, deltaRectangles_in), screen_rect, color_cxt);
//
//     RED_CHECK_IMG(gd, IMG_TEST_PATH "rdp_drawable_5.png");
// }
//
// RED_AUTO_TEST_CASE(TestPngOneRedScreen)
// {
//     auto expected_red =
//     /* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
//     /* 0000 */ "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
//     /* 0000 */ "\x00\x00\x03\x20\x00\x00\x02\x58\x08\x02\x00\x00\x00"             //... ...X.....
//     /* 0000 */ "\x15\x14\x15\x27"                                                 //...'
//     /* 0000 */ "\x00\x00\x0a\xa9\x49\x44\x41\x54"                                 //....IDAT
//     /* 0000 */ "\x78\x9c\xed\xd6\xc1\x09\x00\x20\x10\xc0\x30\x75\xff\x9d\xcf\x25" //x...... ..0u...%
//     /* 0010 */ "\x0a\x82\x24\x13\xf4\xd9\x3d\x0b\x00\x80\xd2\x79\x1d\x00\x00\xf0" //..$...=....y....
//     /* 0020 */ "\x1b\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //.....3X..1....3X
//     /* 0030 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0040 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0050 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0060 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0070 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0080 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0090 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 00a0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 00b0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 00c0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 00d0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 00e0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 00f0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0100 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0110 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0120 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0130 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0140 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0150 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0160 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0170 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0180 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0190 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 01a0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 01b0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 01c0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 01d0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 01e0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 01f0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0200 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0210 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0220 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0230 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0240 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0250 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0260 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0270 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0280 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0290 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 02a0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 02b0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 02c0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 02d0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 02e0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 02f0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0300 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0310 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0320 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0330 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0340 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0350 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0360 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0370 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0380 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0390 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 03a0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 03b0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 03c0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 03d0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 03e0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 03f0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0400 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0410 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0420 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0430 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0440 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0450 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0460 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0470 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0480 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0490 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 04a0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 04b0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 04c0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 04d0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 04e0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 04f0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0500 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0510 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0520 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0530 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0540 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0550 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0560 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0570 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0580 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0590 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 05a0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 05b0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 05c0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 05d0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 05e0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 05f0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0600 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0610 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0620 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0630 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0640 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0650 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0660 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0670 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0680 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0690 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 06a0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 06b0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 06c0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 06d0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 06e0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 06f0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0700 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0710 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0720 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0730 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0740 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0750 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0760 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0770 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0780 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0790 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 07a0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 07b0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 07c0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 07d0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 07e0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 07f0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0800 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0810 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0820 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0830 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0840 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0850 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0860 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0870 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0880 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0890 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 08a0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 08b0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 08c0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 08d0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 08e0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 08f0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0900 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0910 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0920 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0930 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0940 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0950 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0960 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0970 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0980 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0990 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 09a0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 09b0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 09c0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 09d0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 09e0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 09f0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0a00 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0a10 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
//     /* 0a20 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
//     /* 0a30 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
//     /* 0a40 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
//     /* 0a50 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
//     /* 0a60 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
//     /* 0a70 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
//     /* 0a80 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
//     /* 0a90 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
//     /* 0aa0 */ "\x58\x00\x00\xb1\x0b\xbb\xfd\x05\xaf"                             //X........
//     /* 0000 */ "\x0d\x9d\x5e\xa4"                                                 //..^.
//     /* 0000 */ "\x00\x00\x00\x00\x49\x45\x4e\x44"                                 //....IEND
//     /* 0000 */ "\xae\x42\x60\x82"                                                 //.B`.
//     ""_av
//     ;
//
//     // This is how the expected raw PNG (a big flat RED 800x600 screen)
//     // will look as a string
//
//     BufTransport trans;
//     RDPDrawable d(800, 600);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     Rect screen_rect(0, 0, 800, 600);
//     RDPOpaqueRect cmd(Rect(0, 0, 800, 600), encode_color24()(RED));
//     d.draw(cmd, screen_rect, color_cxt);
//     dump_png24(trans, d.impl(), true);
//     RED_CHECK(expected_red == trans.buf);
// }
//
// RED_AUTO_TEST_CASE(TestImageCaptureToFilePngBlueOnRed)
// {
//     BufTransport trans;
//     RDPDrawable drawable(800, 600);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     Rect screen_rect(0, 0, 800, 600);
//     RDPOpaqueRect cmd(Rect(0, 0, 800, 600), encode_color24()(RED));
//     drawable.draw(cmd, screen_rect, color_cxt);
//
//     dump_png24(trans, drawable, true);
//     RED_CHECK_EQUAL(2786, trans.buf.size());
//     trans.buf.clear();
//
//     RDPOpaqueRect cmd2(Rect(50, 50, 100, 50), encode_color24()(BLUE));
//     drawable.draw(cmd2, screen_rect, color_cxt);
//
//     dump_png24(trans, drawable, true);
//     RED_CHECK_EQUAL(2806, trans.buf.size());
// }
//
// RED_AUTO_TEST_CASE(TestSmallImage)
// {
//     BufTransport trans;
//     Rect scr(0, 0, 20, 10);
//     RDPDrawable drawable(20, 10);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     drawable.draw(RDPOpaqueRect(scr, encode_color24()(RED)), scr, color_cxt);
//     drawable.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(BLUE)), scr, color_cxt);
//     drawable.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(WHITE)), scr, color_cxt);
//     dump_png24(trans, drawable, true);
//     RED_CHECK_EQUAL(107, trans.buf.size());
// }
//
// RED_AUTO_TEST_CASE(TestBogusBitmap)
// {
//     Rect scr(0, 0, 800, 600);
//     RDPDrawable drawable(800, 600);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     drawable.draw(RDPOpaqueRect(scr, encode_color24()(GREEN)), scr, color_cxt);
//
//     uint8_t source64x64[] = {
// // MIX_SET 60 remaining=932 bytes pix=0
// /* 0000 */ 0xc0, 0x2c, 0xdf, 0xff,                                      // .,..
// // COPY 6 remaining=931 bytes pix=60
// /* 0000 */ 0x86, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff,           // .............
// // FOM_SET 3006 remaining=913 bytes pix=66
// /* 0000 */ 0xf7, 0xbe, 0x0b, 0x01, 0xF0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // ................
// /* 0010 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0020 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0030 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0040 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0050 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0060 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0070 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0080 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0090 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 00a0 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 00b0 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 00c0 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 00d0 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 00e0 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 00f0 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0100 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0110 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0120 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0130 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0140 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0150 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ....QDDDDDDDDDDD
// /* 0160 */ 0x44, 0x44, 0x44, 0x44, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDDD............
// /* 0170 */ 0x11, 0x11, 0x11, 0x11, 0x51, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x04,           // ....QDDDDDDD.
//
// // BICOLOR 64 remaining=532 bytes pix=3072
// /* 0000 */ 0xe0, 0x10, 0x55, 0xad, 0x35, 0xad,                                // ..U.5.
// // COPY 63 remaining=530 bytes pix=3136
// /* 0000 */ 0x80, 0x1f, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0010 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0020 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0030 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0040 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0050 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0060 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// /* 0070 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff,  // ................
// // COLOR 65 remaining=400 bytes pix=3199
// /* 0000 */ 0x60, 0x21, 0xdf, 0xff,                                      // `!..
// // FOM 387 remaining=396 bytes pix=3264
// /* 0000 */ 0xf2, 0x83, 0x01, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // ................
// /* 0010 */ 0x11, 0x11, 0x11, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  // ...DDDDDDDDDDDDD
// /* 0020 */ 0x44, 0x44, 0x44, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // DDD.............
// /* 0030 */ 0x11, 0x11, 0x11, 0x04,                                      // ....
// // COPY 3 remaining=347 bytes pix=3651
// /* 0000 */ 0x83, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff,                             // .......
// // FOM 122 remaining=338 bytes pix=3654
// /* 0000 */ 0x40, 0x79, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // @y..............
// /* 0010 */ 0x11, 0x01,                                            // ..
// // COPY 99 remaining=321 bytes pix=3776
// /* 0000 */ 0x80, 0x43, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .C..............
// /* 0010 */ 0xdf, 0xff, 0xde, 0xff, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xde, 0xff, 0xdf, 0xff, 0x00, 0x00,  // ................
// /* 0020 */ 0x00, 0x00, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ................
// /* 0030 */ 0xdf, 0xff, 0xde, 0xff, 0x00, 0x00, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0040 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0050 */ 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0060 */ 0x00, 0x00, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0070 */ 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0080 */ 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff,  // ................
// /* 0090 */ 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff,  // ................
// /* 00a0 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00,  // ................
// /* 00b0 */ 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00,  // ................
// /* 00c0 */ 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0x00, 0x00,                          // ........
// // FILL 9 remaining=122 bytes pix=3875
// /* 0000 */ 0x09,                                               // .
// // FOM 63 remaining=119 bytes pix=3884
// /* 0000 */ 0x40, 0x3e, 0x11, 0x11, 0x41, 0x44, 0x04, 0x40, 0x00, 0x44,                    // @>..AD.@.D
// // COPY 15 remaining=111 bytes pix=3947
// /* 0000 */ 0x8f, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde,  // ................
// /* 0010 */ 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff,     // ...............
// // FOM 24 remaining=79 bytes pix=3962
// /* 0000 */ 0x43, 0x11, 0x11, 0x11,                                      // C...
// // COPY 8 remaining=76 bytes pix=3986
// /* 0000 */ 0x88, 0xdf, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, 0xdf,  // ................
// /* 0010 */ 0xff,                                               // .
// // FOM 57 remaining=57 bytes pix=3994
// /* 0000 */ 0x40, 0x38, 0x01, 0x10, 0x11, 0x11, 0x51, 0x44, 0x44, 0x00,                    // @8....QDD.
// // COPY 3 remaining=49 bytes pix=4051
// /* 0000 */ 0x83, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff,                             // .......
// // FILL 10 remaining=42 bytes pix=4054
// /* 0000 */ 0x0a,                                               // .
// // FILL 12 remaining=41 bytes pix=4064
// // magic mix
//
// /* 0000 */ 0x0c,                                               // .
// // COPY 20 remaining=40 bytes pix=4075
// /* 0000 */ 0x94, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf,  // ................
// /* 0010 */ 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf,  // ................
// /* 0020 */ 0xff, 0xde, 0xff, 0xdf, 0xff, 0xdf, 0xff, 0xdf, 0xff,                       // .........
//     }
//     ;
//
//     Bitmap bloc64x64(BitsPerPixel{16}, BitsPerPixel{16}, nullptr, 64, 64, source64x64, sizeof(source64x64), true );
//     drawable.draw(RDPMemBlt(0, Rect(100, 100, bloc64x64.cx(), bloc64x64.cy()), 0xCC, 0, 0, 0), scr, bloc64x64);
//
//     Bitmap good16(BitsPerPixel{24}, bloc64x64);
//     drawable.draw(RDPMemBlt(0, Rect(200, 200, good16.cx(), good16.cy()), 0xCC, 0, 0, 0), scr, good16);
//
//     StaticOutStream<8192> stream;
//     good16.compress(BitsPerPixel{24}, stream);
//     Bitmap bogus(BitsPerPixel{24}, BitsPerPixel{24}, nullptr, 64, 64, stream.get_data(), stream.get_offset(), true);
//     drawable.draw(RDPMemBlt(0, Rect(300, 100, bogus.cx(), bogus.cy()), 0xCC, 0, 0, 0), scr, bogus);
//
//     BufTransport trans;
//     dump_png24(trans, drawable, true);
//     RED_CHECK_EQUAL(4094, trans.buf.size());
// }
//
// RED_AUTO_TEST_CASE(TestBogusBitmap2)
// {
//     Rect scr(0, 0, 800, 600);
//     RDPDrawable drawable(800, 600);
//     auto const color_cxt = gdi::ColorCtx::depth24();
//     drawable.draw(RDPOpaqueRect(scr, encode_color24()(GREEN)), scr, color_cxt);
//
//     uint8_t source32x1[] =
// //MemBlt Primary Drawing Order (0x0D)
// //memblt(id=0 idx=15 x=448 y=335 cx=32 cy=1)
// //Cache Bitmap V2 (Compressed) Secondary Drawing Order (0x05)
// //update_read_cache_bitmap_v2_order
// //update_read_cache_bitmap_v2_order id=0
// //update_read_cache_bitmap_v2_order flags=8 CBR2_NO_BITMAP_COMPRESSION_HDR
// //update_read_cache_bitmap_v2_order width=32 height=1
// //update_read_cache_bitmap_v2_order Index=16
// //rledecompress width=32 height=1 cbSrcBuffer=58
//
// //-- /* BMP Cache compressed V2 */
// //-- COPY1 5
// //-- MIX 1
// //-- COPY1 7
// //-- MIX 1
// //-- COPY1 2
// //-- MIX_SET 4
// //-- COPY1 9
// //-- MIX_SET 3
//
//            "\x85\xf8\xff\x2b\x6a\x6c\x12\x8d\x12\x79\x14"
//            "\x21"
//            "\x87\x15\xff\x2b\x42\x4b\x12\x4c\x12\x6c\x12\x4c\x12\x38\x14"
//            "\x21"
//            "\x82\x32\xfe\x6c\x12"
//            "\xc4\x8d\x12"
//            "\x89\x6d\x12\x4c\x12\x1f\x6e\xff\xff\x2a\xb4\x2b\x12\x6d\x12\xae\x12\xcf\x1a"
//            "\xc3\xef\x1a"
//     ;
//
//     Bitmap bloc32x1(BitsPerPixel{16}, BitsPerPixel{16}, nullptr, 32, 1, source32x1, sizeof(source32x1)-1, true);
//     drawable.draw(RDPMemBlt(0, Rect(100, 100, bloc32x1.cx(), bloc32x1.cy()), 0xCC, 0, 0, 0), scr, bloc32x1);
//
//     BufTransport trans;
//     dump_png24(trans, drawable, true);
//     RED_CHECK_EQUAL(2913, trans.buf.size());
// }
