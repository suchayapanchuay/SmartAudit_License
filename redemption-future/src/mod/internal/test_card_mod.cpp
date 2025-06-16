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
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Javier Caverni, JOnathan Poelen, Raphael Zhou

   Use (implemented) basic RDP orders to draw some known test pattern
*/

#include "gdi/graphic_api.hpp"
#include "gdi/text.hpp"
#include "mod/internal/test_card_mod.hpp"
#include "mod/internal/widget/label.hpp"
#include "core/app_path.hpp"
#include "core/RDP/bitmapupdate.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryLineTo.hpp"
#include "core/RDP/rdp_pointer.hpp"
#include "core/font.hpp"
#include "utils/bitmap_from_file.hpp"
#include "utils/strutils.hpp"


TestCardMod::TestCardMod(gdi::GraphicApi & gd, uint16_t width, uint16_t height, Font const & font)
: front_width(width)
, front_height(height)
, font(font)
, gd(gd)
{
    draw_event();
}

Rect TestCardMod::get_screen_rect() const
{
    return Rect(0, 0, this->front_width, this->front_height);
}

Dimension TestCardMod::get_dim() const
{
    return Dimension(this->front_width, this->front_height);
}

void TestCardMod::rdp_input_scancode(
    KbdFlags flags, Scancode scancode, uint32_t event_time, Keymap const& keymap)
{
    if (pressed_scancode(flags, scancode) == Scancode::Esc) {
        this->set_mod_signal(BACK_EVENT_STOP);
    }
    (void)flags;
    (void)event_time;
    (void)keymap;
}

void TestCardMod::draw_event()
{
    const Rect clip = this->get_screen_rect();

    auto const color_ctx = gdi::ColorCtx::depth24();

    gd.draw(RDPOpaqueRect(this->get_screen_rect(), encode_color24()(NamedBGRColor::WHITE)), clip, color_ctx);
    gd.draw(RDPOpaqueRect(this->get_screen_rect().shrink(5), encode_color24()(NamedBGRColor::RED)), clip, color_ctx);
    gd.draw(RDPOpaqueRect(this->get_screen_rect().shrink(10), encode_color24()(NamedBGRColor::GREEN)), clip, color_ctx);
    gd.draw(RDPOpaqueRect(this->get_screen_rect().shrink(15), encode_color24()(NamedBGRColor::BLUE)), clip, color_ctx);
    gd.draw(RDPOpaqueRect(this->get_screen_rect().shrink(20), encode_color24()(NamedBGRColor::BLACK)), clip, color_ctx);

    Rect winrect = this->get_screen_rect().shrink(30);
    gd.draw(RDPOpaqueRect(winrect, encode_color24()(NamedBGRColor::WINBLUE)), clip, color_ctx);


    Bitmap bitmap = bitmap_from_file(str_concat(app_path(AppPath::Share), "/" "Philips_PM5544_640.png").c_str(), NamedBGRColor::BLACK);

    gd.draw(RDPMemBlt(0,
        Rect(winrect.x + (winrect.cx - bitmap.cx())/2,
                winrect.y + (winrect.cy - bitmap.cy())/2,
                bitmap.cx(), bitmap.cy()),
                0xCC,
            0, 0, 0), clip, bitmap);

    //  lineTo mix_mode=1 startx=200 starty=1198 endx=200 endy=145 bg_color=0 rop2=13 clip=(200, 145, 1, 110)
    gd.draw(
        RDPLineTo(1, 200, 1198, 200, 145, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(200, 145, 1, 110), color_ctx);

    gd.draw(
        RDPLineTo(1, 200, 145, 200, 1198, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(200, 145, 1, 110), color_ctx);

    gd.draw(
        RDPLineTo(1, 201, 1198, 200, 145, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(200, 145, 1, 110), color_ctx);

    gd.draw(
        RDPLineTo(1, 200, 145, 201, 1198, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(200, 145, 1, 110), color_ctx);

    gd.draw(
        RDPLineTo(1, 1198, 200, 145, 200, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(145, 200, 110, 1), color_ctx);

    gd.draw(
        RDPLineTo(1, 145, 200, 1198, 200, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(145, 200, 110, 1), color_ctx);

    gd.draw(
        RDPLineTo(1, 1198, 201, 145, 200, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(145, 200, 110, 1), color_ctx);

    gd.draw(
        RDPLineTo(1, 145, 200, 1198, 201, RDPColor{}, 13, RDPPen(0, 1, encode_color24()(NamedBGRColor::RED))),
        Rect(145, 200, 110, 1), color_ctx);

    auto draw_text = [&](int x, int y, chars_view str, NamedBGRColor fg, NamedBGRColor bg){
        gdi::draw_text(
            gd, x, y, font.max_height(), gdi::DrawTextPadding{},
            WidgetText<32>(this->font, str).fcs(),
            encode_color24()(fg), encode_color24()(bg),
            clip
        );
    };

    draw_text(30, 30, "White"_av, NamedBGRColor::WHITE, NamedBGRColor::BLACK);
    draw_text(30, 50, "Red  "_av, NamedBGRColor::RED, NamedBGRColor::BLACK);
    draw_text(30, 70, "Green"_av, NamedBGRColor::GREEN, NamedBGRColor::BLACK);
    draw_text(30, 90, "Blue "_av, NamedBGRColor::BLUE, NamedBGRColor::BLACK);
    draw_text(30, 110, "Black"_av, NamedBGRColor::BLACK, NamedBGRColor::WHITE);

    Bitmap card = bitmap_from_file(app_path(AppPath::RedemptionLogo24), NamedBGRColor::BLACK);
    gd.draw(RDPMemBlt(0,
        Rect(this->get_screen_rect().cx - card.cx() - 30,
                this->get_screen_rect().cy - card.cy() - 30, card.cx(), card.cy()),
                0xCC,
            0, 0, 0), clip, card);

    // Bogus square generating zero width/height tiles if not properly guarded
    uint8_t comp64x64RED[] = {
        0xc0, 0x30, 0x00, 0x00, 0xFF,
        0xf0, 0xc0, 0x0f,
    };

    Bitmap bloc64x64(BitsPerPixel{24}, BitsPerPixel{24}, &BGRPalette::classic_332(), 64, 64, comp64x64RED, sizeof(comp64x64RED), true);
    gd.draw(RDPMemBlt(0,
        Rect(0, this->get_screen_rect().cy - 64, bloc64x64.cx(), bloc64x64.cy()), 0xCC,
            32, 32, 0), clip, bloc64x64);

    //gd.draw(RDPOpaqueRect(this->get_screen_rect(), NamedBGRColor::RED), clip, depth);
    Bitmap wab_logo_blue = bitmap_from_file(app_path(AppPath::LoginWabBlue), NamedBGRColor::BLACK);


    const uint16_t startx = 5;
    const uint16_t starty = 5;

    const uint16_t tile_width_height = 32;

    for (uint16_t y = 0; y < wab_logo_blue.cy(); y += tile_width_height) {
        uint16_t cy = std::min<uint16_t>(tile_width_height, wab_logo_blue.cy() - y);

        for (uint16_t x = 0; x < wab_logo_blue.cx(); x += tile_width_height) {
            uint16_t cx = std::min<uint16_t>(tile_width_height, wab_logo_blue.cx() - x);

            Bitmap tile(wab_logo_blue, Rect(x, y, cx, cy));

            RDPBitmapData bitmap_data;

            bitmap_data.dest_left       = startx + x;
            bitmap_data.dest_top        = starty + y;
            bitmap_data.dest_right      = bitmap_data.dest_left + cx - 1;
            bitmap_data.dest_bottom     = bitmap_data.dest_top + cy - 1;
            bitmap_data.width           = tile.cx();
            bitmap_data.height          = tile.cy();
            bitmap_data.bits_per_pixel  = 24;
            bitmap_data.flags           = 0;
            bitmap_data.bitmap_length   = tile.bmp_size();

            // bitmap_data.log(LOG_INFO);

            gd.draw(bitmap_data, tile);
        }
    }

    uint8_t pointer_data[] =
/* 0000 */ "\x02\x00\x02\x00\x05\x00\x05\x00\x0a\x00\x64\x00\x00\x00\x00\x00" // ..........d.....
/* 0010 */ "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00" // ................
/* 0020 */ "\xff\xff\xff\xff\x00\x00\x00\xff\x00\x00\x00\xff\x00\x00\x00\xff" // ................
/* 0030 */ "\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\xff\x00\x00\x00\xff" // ................
/* 0040 */ "\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\xff" // ................
/* 0050 */ "\x00\x00\x00\xff\x00\x00\x00\xff\xff\xff\xff\xff\x00\x00\x00\x00" // ................
/* 0060 */ "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00" // ................
/* 0070 */ "\x88\x00\x00\x00\x00\x00\x00\x00\x88\x00"                         // ..........
        ;
    InStream pointer_stream(pointer_data);
    RdpPointerView pointer = pointer_loader_new(BitsPerPixel{32}, pointer_stream);

    auto cache_idx = gdi::CachePointerIndex(9);
    gd.new_pointer(cache_idx, pointer);
    gd.cached_pointer(cache_idx);
}
