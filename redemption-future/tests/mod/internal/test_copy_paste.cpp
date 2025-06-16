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
 *   Copyright (C) Wallix 2013
 *   Author(s): Christophe Grosjean
 *
 */

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/check_img.hpp"
#include "test_only/front/fake_front.hpp"
#include "test_only/gdi/test_graphic.hpp"
#include "test_only/core/font.hpp"

#include "core/RDP/clipboard.hpp"
#include "core/RDP/clipboard/format_list_serialize.hpp"
#include "mod/internal/copy_paste.hpp"
#include "mod/internal/widget/edit.hpp"

#include <string>

using namespace std::string_view_literals;


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/mod/internal/widget/copy_paste/"


struct CopyPasteFront : FakeFront
{
    CopyPasteFront(ScreenInfo & info, CopyPaste & copy_paste)
    : FakeFront(info)
    , copy_paste(copy_paste)
    {
        this->get_writable_channel_list().push_back(CHANNELS::ChannelDef(channel_names::cliprdr, 0, 0));
    }

    void send_to_channel(
        const CHANNELS::ChannelDef& channel, bytes_view chunk_data,
        std::size_t /*total_length*/, uint32_t /*flags*/
    ) override {
        RED_REQUIRE(channel.name == channel_names::cliprdr);

        InStream stream(chunk_data);
        RDPECLIP::RecvPredictor rp(stream);

        switch (rp.msgType()) {
            case RDPECLIP::CB_MONITOR_READY:
            {
                StaticOutStream<1600> out_s;
                Cliprdr::format_list_serialize_with_header(
                    out_s, Cliprdr::IsLongFormat(false),
                    std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

                InStream in_s(out_s.get_produced_bytes());
                this->copy_paste.send_to_mod_channel(in_s, CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST);
            }

            break;
            //case RDPECLIP::CB_FORMAT_LIST:
            //    break;
            //case RDPECLIP::CB_FORMAT_LIST_RESPONSE:
            //    break;

            case RDPECLIP::CB_FORMAT_DATA_REQUEST:
            {
                RDPECLIP::FormatDataRequestPDU().recv(stream);

                StaticOutStream<256> out_s;
                OutStream out_header = out_s.stream_at(0);

                RDPECLIP::CliprdrHeader().emit(out_s); // skip header

                uint32_t data_len = UTF8toUTF16(this->str, out_s.get_tail());
                out_s.out_skip_bytes(data_len);
                out_s.out_clear_bytes(2);  // null-terminated char (UTF-16)

                RDPECLIP::CliprdrHeader(
                    RDPECLIP::CB_FORMAT_DATA_RESPONSE, RDPECLIP::CB_RESPONSE_OK,
                    data_len + 2
                ).emit(out_header);

                InStream in_s(out_s.get_produced_bytes());
                this->copy_paste.send_to_mod_channel(in_s, CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST);
            }
            break;

            default:
            break;
        }
    }

    void copy(const char * s) {
        this->str = s;

        StaticOutStream<1600> out_s;
        Cliprdr::format_list_serialize_with_header(
            out_s, Cliprdr::IsLongFormat(false),
            std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

        InStream in_s(out_s.get_produced_bytes());
        this->copy_paste.send_to_mod_channel(in_s, CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST);
    }

private:
//    template<class PDU, class... Args>
//    void send_to_server(PDU && pdu, Args && ...args) {
//        StaticOutStream<256> out_s;
//        pdu.emit(out_s, std::move(args)...);
//        InStream in_s(out_s.get_produced_bytes());
//        this->copy_paste.send_to_mod_channel(in_s, CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST);
//    }

    CopyPaste & copy_paste;
    std::string str;
};

RED_AUTO_TEST_CASE(TestPaste)
{
    ScreenInfo screen_info{120, 20, BitsPerPixel{24}};

    CopyPaste copy_paste{false};
    CopyPasteFront front(screen_info, copy_paste);
    TestGraphic gd(screen_info.width, screen_info.height);

    WidgetEdit edit(
        gd, global_font_deja_vu_14(), copy_paste,
        {
            .fg = NamedBGRColor::PINK,
            .bg = NamedBGRColor::ORANGE,
            .border = NamedBGRColor::ORANGE,
            .focus_border = NamedBGRColor::RED,
            .cursor = Widget::Color(0x888888),
        },
        WidgetEventNotifier()
    );
    edit.update_width(120);
    edit.rdp_input_invalidate(edit.get_rect());

    RED_REQUIRE(copy_paste.ready(front));

    #define edit_paste(s, imgref) do {             \
        copy_paste.paste(edit);                    \
        RED_CHECK_EQUAL(s ""_av, edit.get_text()); \
        RED_CHECK_IMG(gd, imgref);                 \
    } while (0)
    edit_paste("", IMG_TEST_PATH "empty.png");
    edit_paste("", IMG_TEST_PATH "empty.png");
    front.copy("plop");
    edit_paste("plop", IMG_TEST_PATH "plop.png");
    edit.action_move_cursor_left(false, WidgetEdit::Redraw::Yes);
    edit_paste("ploplopp", IMG_TEST_PATH "plopplop.png");
    front.copy("xxx");
    edit_paste("ploplopxxxp", IMG_TEST_PATH "ploplopxxxp.png");
    edit_paste("ploplopxxxxxxp", IMG_TEST_PATH "ploplopxxxxxxp.png");
    edit.set_text(""_av, {WidgetEdit::Redraw::Yes});
    front.copy("abc\tde");
    edit_paste("abc de", IMG_TEST_PATH "abcde.png");
    front.copy("fg\nhi");
    edit_paste("abc defg hi", IMG_TEST_PATH "abcdefg.png");
    front.copy("jk\tl\nmn");
    edit_paste("abc defg hijk l mn", IMG_TEST_PATH "abcdefgjkl.png");
}
