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
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   Unit test to conversion of RDP drawing orders to PNG images
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"
#include "test_only/test_framework/check_img.hpp"
#include "test_only/test_framework/file.hpp"
#include "test_only/force_paris_timezone.hpp"
#include "test_only/session_log_test.hpp"
#include "test_only/lcg_random.hpp"

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wheader-hygiene")
#include "capture/capture.cpp" /* NOLINT(bugprone-suspicious-include) */
REDEMPTION_DIAGNOSTIC_POP()

#include "capture/capture.hpp"
#include "capture/file_to_graphic.hpp"
#include "capture/full_video_params.hpp"
#include "capture/sequenced_video_params.hpp"
#include "transport/file_transport.hpp"
#include "utils/drawable.hpp"
#include "utils/fileutils.hpp"
#include "utils/stream.hpp"
#include "utils/strutils.hpp"
#include "utils/bitmap_from_file.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "test_only/ostream_buffered.hpp"
#include "test_only/transport/test_transport.hpp"


#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/capture"

using namespace std::chrono_literals;

static struct stat get_stat(char const* filename)
{
    class stat st;
    stat(filename, &st);
    return st;
}


namespace
{
    struct TestCaptureContext
    {
        // Timestamps are applied only when flushing
        MonotonicTimePoint now{1000s};

        LCGRandom rnd;
        CryptoContext cctx;
        NullSessionLog session_log;

        bool capture_wrm;
        bool capture_png;

        bool capture_pattern_checker = false;
        bool capture_ocr = false;
        bool capture_video = false;
        bool capture_video_full = false;
        bool capture_meta = false;
        bool capture_kbd;

        std::array<RdpPointer, PointerCache::MAX_POINTER_COUNT> pointers;

        MetaParams meta_params {};
        VideoParams video_params {};
        PatternParams pattern_params {}; // reading with capture_kbd = true
        FullVideoParams full_video_params {};
        SequencedVideoParams sequenced_video_params {};
        KbdLogParams kbd_log_params {};
        OcrParams const ocr_params {
            OcrVersion::v1, ocr::locale::LocaleId::latin,
            false, 0, std::chrono::seconds::zero(), 0};

        PngParams png_params = {0, 0, std::chrono::milliseconds{60}, 0, false, false, false, nullptr, {}};

        const char * record_tmp_path;
        const char * record_path;
        const char * hash_path;

        RDPDrawable rdp_drawable;

        DrawableParams const drawable_params;

        WrmParams const wrm_params;

        CaptureParams capture_params;

        TestCaptureContext(
            char const* basename, CaptureFlags capture_flags, uint16_t cx, uint16_t cy,
            WorkingDirectory& record_wd, WorkingDirectory& hash_wd,
            KbdLogParams kbd_log_params)
        : capture_wrm(bool(capture_flags & CaptureFlags::wrm))
        , capture_png(bool(capture_flags & CaptureFlags::png))
        , capture_kbd(kbd_log_params.wrm_keyboard_log)
        , kbd_log_params(kbd_log_params)
        , record_tmp_path(record_wd.dirname())
        , record_path(record_tmp_path)
        , hash_path(hash_wd.dirname())
        , rdp_drawable(cx, cy)
        , drawable_params{
            rdp_drawable,
            PointerCache::SourcePointersView{pointers},
        }
        , wrm_params{
            BitsPerPixel{24},
            false,
            cctx,
            rnd,
            hash_path,
            std::chrono::seconds{3},
            WrmCompressionAlgorithm::no_compression,
            RDPSerializerVerbose::none,
        }
        , capture_params{
            now,
            RealTimePoint{1000s},
            basename,
            record_tmp_path,
            record_path,
            FilePermissions::user_permissions(BitPermissions::read),
            &session_log,
            SmartVideoCropping::disable,
            0
        }
        {
            cctx.set_trace_type(TraceType::localfile);
            this->png_params.real_basename = basename;
        }

        template<class F>
        void run(F&& f)
        {
            Capture capture(
                capture_params, drawable_params,
                capture_wrm, wrm_params,
                capture_png, png_params,
                capture_pattern_checker, pattern_params,
                capture_ocr, ocr_params,
                capture_video, sequenced_video_params,
                capture_video_full, full_video_params,
                capture_meta, meta_params,
                capture_kbd, kbd_log_params,
                video_params, nullptr, Capture::CropperInfo(), Rect()
            );

            f(capture, Rect(0, 0, rdp_drawable.width(), rdp_drawable.height()));
        }
    };

    template<class F>
    void test_capture_context(
        char const* basename, CaptureFlags capture_flags, uint16_t cx, uint16_t cy,
        WorkingDirectory& record_wd, WorkingDirectory& hash_wd, KbdLogParams kbd_log_params,
        F&& f)
    {
        TestCaptureContext{basename, capture_flags, cx, cy, record_wd, hash_wd, kbd_log_params}
          .run(f);
    }

    void capture_draw_color1(MonotonicTimePoint& now, Capture& capture, Rect scr, uint16_t cy)
    {
        auto const color_cxt = gdi::ColorCtx::depth24();

        capture.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::GREEN)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);

        capture.draw(RDPOpaqueRect(Rect(1, 50, cy, 30), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);

        capture.draw(RDPOpaqueRect(Rect(2, 100, cy, 30), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);

        capture.draw(RDPOpaqueRect(Rect(3, 150, cy, 30), encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);
    }

    void capture_draw_color2(MonotonicTimePoint& now, Capture& capture, Rect scr, uint16_t cy)
    {
        auto const color_cxt = gdi::ColorCtx::depth24();

        capture.draw(RDPOpaqueRect(Rect(4, 200, cy, 30), encode_color24()(NamedBGRColor::BLACK)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);

        capture.draw(RDPOpaqueRect(Rect(5, 250, cy, 30), encode_color24()(NamedBGRColor::PINK)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);

        capture.draw(RDPOpaqueRect(Rect(6, 300, cy, 30), encode_color24()(NamedBGRColor::WABGREEN)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);
    }

    inline time_t to_time_t(MonotonicTimePoint t)
    {
        auto duration = t.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    }

    inline time_t to_time_t(RealTimePoint t)
    {
        auto duration = t.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    }
} // namespace


RED_AUTO_TEST_CASE(TestSplittedCapture)
{
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    force_paris_timezone();

    test_capture_context("test_capture", CaptureFlags::wrm | CaptureFlags::png,
        800, 600, record_wd, hash_wd, KbdLogParams(), [](Capture& capture, Rect scr)
    {
        // Timestamps are applied only when flushing
        MonotonicTimePoint now{1000s};

        capture_draw_color1(now, capture, scr, 700);
        capture_draw_color2(now, capture, scr, 700);
    });

    auto mwrm = record_wd.add_file("test_capture.mwrm");
    auto wrm0 = record_wd.add_file("test_capture-000000.wrm");
    auto wrm1 = record_wd.add_file("test_capture-000001.wrm");
    auto wrm2 = record_wd.add_file("test_capture-000002.wrm");
    auto st = get_stat(mwrm);
    auto st0 = get_stat(wrm0);
    auto st1 = get_stat(wrm1);
    auto st2 = get_stat(wrm2);
    RED_TEST_FILE_SIZE(wrm0, 1670);
    RED_TEST_FILE_SIZE(wrm1, 3553);
    RED_TEST_FILE_SIZE(wrm2, 3508);
    RED_TEST_FILE_CONTENTS(mwrm, array_view{str_concat(
        "v2\n"
        "800 600\n"
        "nochecksum\n"
        "\n"
        "\n",
        wrm0, " 1670 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), " 1000 1004\n",
        wrm1, " 3553 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), " 1004 1007\n",
        wrm2, " 3508 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), " 1007 1008\n")});

    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000000.png"), 3097);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000001.png"), 3123);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000002.png"), 3139);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000003.png"), 3155);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000004.png"), 3170);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000005.png"), 3196);
    RED_TEST_FILE_SIZE(record_wd.add_file("test_capture-000006.png"), 3220);

    RED_TEST_FILE_CONTENTS(hash_wd.add_file("test_capture-000000.wrm"), array_view{str_concat(
        "v2\n\n\ntest_capture-000000.wrm 1670 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("test_capture-000001.wrm"), array_view{str_concat(
        "v2\n\n\ntest_capture-000001.wrm 3553 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("test_capture-000002.wrm"), array_view{str_concat(
        "v2\n\n\ntest_capture-000002.wrm 3508 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), '\n')});

    RED_TEST_FILE_CONTENTS(hash_wd.add_file("test_capture.mwrm"), array_view{str_concat(
        "v2\n\n\ntest_capture.mwrm ",
        int_to_decimal_chars(st.st_size), ' ',
        int_to_decimal_chars(st.st_mode), ' ',
        int_to_decimal_chars(st.st_uid), ' ',
        int_to_decimal_chars(st.st_gid), ' ',
        int_to_decimal_chars(st.st_dev), ' ',
        int_to_decimal_chars(st.st_ino), ' ',
        int_to_decimal_chars(st.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st.st_ctim.tv_sec), '\n')});

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}

RED_AUTO_TEST_CASE(TestBppToOtherBppCapture)
{
    force_paris_timezone();
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    test_capture_context("test_capture", CaptureFlags::png,
        100, 100, record_wd, hash_wd, KbdLogParams(), [](Capture& capture, Rect scr)
    {
        // Timestamps are applied only when flushing
        MonotonicTimePoint now{1000s};

        auto const color_cxt = gdi::ColorCtx::depth16();
        capture.cached_pointer(PredefinedPointer::Edit);

        capture.draw(RDPOpaqueRect(scr, encode_color16()(NamedBGRColor::BLUE)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 15, 21);
    });

    RED_CHECK_IMG(record_wd.add_file("test_capture-000000.png"),
                  IMG_TEST_PATH "/bpp_to_other_bpp.png");

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}

RED_AUTO_TEST_CASE(TestResizingCapture)
{
    force_paris_timezone();
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    test_capture_context("resizing-capture-0", CaptureFlags::wrm | CaptureFlags::png,
        800, 600, record_wd, hash_wd, KbdLogParams(), [](Capture& capture, Rect scr)
    {
        // Timestamps are applied only when flushing
        MonotonicTimePoint now{1000s};

        capture_draw_color1(now, capture, scr, 1200);

        scr.cx = 1024;
        scr.cy = 768;

        capture.resize(scr.cx, scr.cy);

        capture_draw_color2(now, capture, scr, 1200);

        auto const color_cxt = gdi::ColorCtx::depth24();

        capture.draw(RDPOpaqueRect(Rect(7, 350, 1200, 30), encode_color24()(NamedBGRColor::YELLOW)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);
    });

    auto mwrm = record_wd.add_file("resizing-capture-0.mwrm");
    auto wrm0 = record_wd.add_file("resizing-capture-0-000000.wrm");
    auto wrm1 = record_wd.add_file("resizing-capture-0-000001.wrm");
    auto wrm2 = record_wd.add_file("resizing-capture-0-000002.wrm");
    auto wrm3 = record_wd.add_file("resizing-capture-0-000003.wrm");
    auto st = get_stat(mwrm);
    auto st0 = get_stat(wrm0);
    auto st1 = get_stat(wrm1);
    auto st2 = get_stat(wrm2);
    auto st3 = get_stat(wrm3);
    RED_TEST_FILE_SIZE(wrm0, 1675);
    RED_TEST_FILE_SIZE(wrm1, 3473);
    RED_TEST_FILE_SIZE(wrm2, 4429);
    RED_TEST_FILE_SIZE(wrm3, 4433);
    RED_TEST_FILE_CONTENTS(mwrm, array_view{str_concat(
        "v2\n"
        "800 600\n"
        "nochecksum\n"
        "\n"
        "\n",
        wrm0, " 1675 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), " 1000 1004\n",
        wrm1, " 3473 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), " 1004 1005\n",
        wrm2, " 4429 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), " 1005 1007\n",
        wrm3, " 4433 ",
        int_to_decimal_chars(st3.st_mode), ' ',
        int_to_decimal_chars(st3.st_uid), ' ',
        int_to_decimal_chars(st3.st_gid), ' ',
        int_to_decimal_chars(st3.st_dev), ' ',
        int_to_decimal_chars(st3.st_ino), ' ',
        int_to_decimal_chars(st3.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st3.st_ctim.tv_sec), " 1007 1009\n")});

    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000000.png"), 3102 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000001.png"), 3121 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000002.png"), 3131 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000003.png"), 3143 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000004.png"), 4079 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000005.png"), 4103 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000006.png"), 4122 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-0-000007.png"), 4137 +- 100_v);

    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-0-000000.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-0-000000.wrm 1675 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-0-000001.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-0-000001.wrm 3473 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-0-000002.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-0-000002.wrm 4429 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-0-000003.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-0-000003.wrm 4433 ",
        int_to_decimal_chars(st3.st_mode), ' ',
        int_to_decimal_chars(st3.st_uid), ' ',
        int_to_decimal_chars(st3.st_gid), ' ',
        int_to_decimal_chars(st3.st_dev), ' ',
        int_to_decimal_chars(st3.st_ino), ' ',
        int_to_decimal_chars(st3.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st3.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-0.mwrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-0.mwrm ",
        int_to_decimal_chars(st.st_size), ' ',
        int_to_decimal_chars(st.st_mode), ' ',
        int_to_decimal_chars(st.st_uid), ' ',
        int_to_decimal_chars(st.st_gid), ' ',
        int_to_decimal_chars(st.st_dev), ' ',
        int_to_decimal_chars(st.st_ino), ' ',
        int_to_decimal_chars(st.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st.st_ctim.tv_sec), '\n')});

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}

RED_AUTO_TEST_CASE(TestResizingCapture1)
{
    force_paris_timezone();
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    test_capture_context("resizing-capture-1", CaptureFlags::wrm | CaptureFlags::png,
        800, 600, record_wd, hash_wd, KbdLogParams(), [](Capture& capture, Rect scr)
    {
        MonotonicTimePoint now{1000s};

        capture_draw_color1(now, capture, scr, 700);

        capture.resize(640, 480);

        capture_draw_color2(now, capture, scr, 700);

        auto const color_cxt = gdi::ColorCtx::depth24();

        capture.draw(RDPOpaqueRect(Rect(7, 350, 700, 30), encode_color24()(NamedBGRColor::YELLOW)), scr, color_cxt);
        now += 1s;
        capture.force_flush(now, 0, 0);
        capture.periodic_snapshot(now, 0, 0);
    });

    auto mwrm = record_wd.add_file("resizing-capture-1.mwrm");
    auto wrm0 = record_wd.add_file("resizing-capture-1-000000.wrm");
    auto wrm1 = record_wd.add_file("resizing-capture-1-000001.wrm");
    auto wrm2 = record_wd.add_file("resizing-capture-1-000002.wrm");
    auto wrm3 = record_wd.add_file("resizing-capture-1-000003.wrm");
    auto st = get_stat(mwrm);
    auto st0 = get_stat(wrm0);
    auto st1 = get_stat(wrm1);
    auto st2 = get_stat(wrm2);
    auto st3 = get_stat(wrm3);
    RED_TEST_FILE_SIZE(wrm0, 1670);
    RED_TEST_FILE_SIZE(wrm1, 3484);
    RED_TEST_FILE_SIZE(wrm2, 2675);
    RED_TEST_FILE_SIZE(wrm3, 2675);
    RED_TEST_FILE_CONTENTS(mwrm, array_view{str_concat(
        "v2\n"
        "800 600\n"
        "nochecksum\n"
        "\n"
        "\n",
        wrm0, " 1670 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), " 1000 1004\n",
        wrm1, " 3484 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), " 1004 1005\n",
        wrm2, " 2675 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), " 1005 1007\n",
        wrm3, " 2675 ",
        int_to_decimal_chars(st3.st_mode), ' ',
        int_to_decimal_chars(st3.st_uid), ' ',
        int_to_decimal_chars(st3.st_gid), ' ',
        int_to_decimal_chars(st3.st_dev), ' ',
        int_to_decimal_chars(st3.st_ino), ' ',
        int_to_decimal_chars(st3.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st3.st_ctim.tv_sec), " 1007 1009\n")});

    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000000.png"), 3102 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000001.png"), 3127 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000002.png"), 3145 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000003.png"), 3162 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000004.png"), 2304 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000005.png"), 2320 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000006.png"), 2334 +- 100_v);
    RED_TEST_FILE_SIZE(record_wd.add_file("resizing-capture-1-000007.png"), 2345 +- 100_v);

    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-1-000000.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-1-000000.wrm 1670 ",
        int_to_decimal_chars(st0.st_mode), ' ',
        int_to_decimal_chars(st0.st_uid), ' ',
        int_to_decimal_chars(st0.st_gid), ' ',
        int_to_decimal_chars(st0.st_dev), ' ',
        int_to_decimal_chars(st0.st_ino), ' ',
        int_to_decimal_chars(st0.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st0.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-1-000001.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-1-000001.wrm 3484 ",
        int_to_decimal_chars(st1.st_mode), ' ',
        int_to_decimal_chars(st1.st_uid), ' ',
        int_to_decimal_chars(st1.st_gid), ' ',
        int_to_decimal_chars(st1.st_dev), ' ',
        int_to_decimal_chars(st1.st_ino), ' ',
        int_to_decimal_chars(st1.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st1.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-1-000002.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-1-000002.wrm 2675 ",
        int_to_decimal_chars(st2.st_mode), ' ',
        int_to_decimal_chars(st2.st_uid), ' ',
        int_to_decimal_chars(st2.st_gid), ' ',
        int_to_decimal_chars(st2.st_dev), ' ',
        int_to_decimal_chars(st2.st_ino), ' ',
        int_to_decimal_chars(st2.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st2.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-1-000003.wrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-1-000003.wrm 2675 ",
        int_to_decimal_chars(st3.st_mode), ' ',
        int_to_decimal_chars(st3.st_uid), ' ',
        int_to_decimal_chars(st3.st_gid), ' ',
        int_to_decimal_chars(st3.st_dev), ' ',
        int_to_decimal_chars(st3.st_ino), ' ',
        int_to_decimal_chars(st3.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st3.st_ctim.tv_sec), '\n')});
    RED_TEST_FILE_CONTENTS(hash_wd.add_file("resizing-capture-1.mwrm"), array_view{str_concat(
        "v2\n\n\nresizing-capture-1.mwrm ",
        int_to_decimal_chars(st.st_size), ' ',
        int_to_decimal_chars(st.st_mode), ' ',
        int_to_decimal_chars(st.st_uid), ' ',
        int_to_decimal_chars(st.st_gid), ' ',
        int_to_decimal_chars(st.st_dev), ' ',
        int_to_decimal_chars(st.st_ino), ' ',
        int_to_decimal_chars(st.st_mtim.tv_sec), ' ',
        int_to_decimal_chars(st.st_ctim.tv_sec), '\n')});

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}

RED_AUTO_TEST_CASE(TestPatternOcr)
{
    force_paris_timezone();
    CapturePattern cap_pattern{
        CapturePattern::Filters{.is_ocr = true, .is_kbd = false},
        CapturePattern::PatternType::reg,
        ".de."_av
    };

    // notify
    {
        SessionLogTest report_message;
        Capture::PatternsChecker checker(report_message, {}, {&cap_pattern, 1u});

        checker.title_changed("Gestionnaire"_av);
        RED_CHECK(report_message.events() == ""_av);

        checker.title_changed("Gestionnaire de serveur"_av);
        RED_CHECK(report_message.events() ==
            "NOTIFY_PATTERN_DETECTED pattern=\"$ocr:.de.\" data=\"Gestionnaire de serveur\"\n"
            "FINDPATTERN_NOTIFY: $ocr:.de.\x02""Gestionnaire de serveur\n"
            ""_av);

        checker.title_changed("Gestionnaire de licences TS"_av);
        RED_CHECK(report_message.events() ==
            "NOTIFY_PATTERN_DETECTED pattern=\"$ocr:.de.\" data=\"Gestionnaire de licences TS\"\n"
            "FINDPATTERN_NOTIFY: $ocr:.de.\x02""Gestionnaire de licences TS\n"
            ""_av);
    }

    // kill
    {
        SessionLogTest report_message;
        Capture::PatternsChecker checker(report_message, {&cap_pattern, 1u}, {});

        checker.title_changed("Gestionnaire"_av);
        RED_CHECK(report_message.events() == ""_av);

        checker.title_changed("Gestionnaire de serveur"_av);
        RED_CHECK(report_message.events() ==
            "KILL_PATTERN_DETECTED pattern=\"$ocr:.de.\" data=\"Gestionnaire de serveur\"\n"
            "FINDPATTERN_KILL: $ocr:.de.\x02""Gestionnaire de serveur\n"
            ""_av);

        checker.title_changed("Gestionnaire de licences TS"_av);
        RED_CHECK(report_message.events() ==
            "KILL_PATTERN_DETECTED pattern=\"$ocr:.de.\" data=\"Gestionnaire de licences TS\"\n"
            "FINDPATTERN_KILL: $ocr:.de.\x02""Gestionnaire de licences TS\n"
            ""_av);
    }
}


namespace
{
    MetaParams make_meta_params()
    {
        return MetaParams{
            MetaParams::EnableSessionLog::No,
            MetaParams::HideNonPrintable::No,
            MetaParams::LogClipboardActivities::Yes,
            MetaParams::LogFileSystemActivities::Yes,
            MetaParams::LogOnlyRelevantClipboardActivities::Yes
        };
    }

    Capture::SessionMeta make_session_meta(MonotonicTimePoint now, Transport& trans, bool key_markers_hidden_state)
    {
        return Capture::SessionMeta(
            now, RealTimePoint{now.time_since_epoch()},
            trans, key_markers_hidden_state, make_meta_params());
    }
} // namespace


RED_AUTO_TEST_CASE(TestSessionMeta)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        auto send_kbd = [&]{
            meta.kbd_input(now, 'A');
            meta.kbd_input(now, 'B');
            meta.kbd_input(now, 'C');
            meta.kbd_input(now, 'D');
            now += 1s;
        };

        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        send_kbd();
        meta.periodic_snapshot(now, 0, 0);
        send_kbd();
        meta.title_changed(now, "Blah1"_av);
        now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av);
        now += 1s;
        send_kbd();
        send_kbd();
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av);
        now += 1s;
        meta.periodic_snapshot(now, 0, 0);
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:50 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:51 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:54 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:55 - type=\"KBD_INPUT\" data=\"ABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMetaQuoted)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        auto send_kbd = [&]{
            meta.kbd_input(now, 'A');
            meta.kbd_input(now, '\"');
            meta.kbd_input(now, 'C');
            meta.kbd_input(now, 'D');
            now += 1s;
        };
        send_kbd();
        meta.periodic_snapshot(now, 0, 0);
        send_kbd();
        meta.title_changed(now, "Bl\"ah1"_av);
        now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah\\2"_av);
        meta.session_update(now, LogId::INPUT_LANGUAGE, {
            KVLog("identifier"_av, "fr"_av),
            KVLog("display_name"_av, "xy\\z"_av),
        });
        now += 1s;
        meta.periodic_snapshot(now, 0, 0);
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:42 + type=\"TITLE_BAR\" data=\"Bl\\\"ah1\"\n"
        "1970-01-01 01:16:43 + type=\"TITLE_BAR\" data=\"Blah\\\\2\"\n"
        "1970-01-01 01:16:43 - type=\"INPUT_LANGUAGE\" identifier=\"fr\" display_name=\"xy\\\\z\"\n"
        "1970-01-01 01:16:44 - type=\"KBD_INPUT\" data=\"A\\\"CDA\\\"CD\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMeta2)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        auto send_kbd = [&]{
            meta.kbd_input(now, 'A');
            meta.kbd_input(now, 'B');
            meta.kbd_input(now, 'C');
            meta.kbd_input(now, 'D');
        };

        meta.title_changed(now, "Blah1"_av); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av); now += 1s;
        send_kbd(); now += 1s;
        send_kbd(); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.next_video(now);
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:40 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:41 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:44 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:45 + (break)\n"
        "1970-01-01 01:16:45 - type=\"KBD_INPUT\" data=\"ABCDABCD\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMeta3)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        auto send_kbd = [&]{
            meta.kbd_input(now, 'A');
            meta.kbd_input(now, 'B');
            meta.kbd_input(now, 'C');
            meta.kbd_input(now, 'D');
        };

        send_kbd(); now += 1s;

        meta.title_changed(now, "Blah1"_av); now += 1s;

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, ""_av),
            KVLog("button"_av, "Démarrer"_av),
        });
        now += 1s;

        meta.session_update(now, LogId::CHECKBOX_CLICKED, {
            KVLog("windows"_av, "User Properties"_av),
            KVLog("checkbox"_av, "User cannot change password"_av),
            KVLog("state"_av, "checked"_av),
        });
        now += 1s;

        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av); now += 1s;
        send_kbd(); now += 1s;
        send_kbd(); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.next_video(now);
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:41 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:42 - type=\"BUTTON_CLICKED\" windows=\"\" button=\"Démarrer\"\n"
        "1970-01-01 01:16:43 - type=\"CHECKBOX_CLICKED\" windows=\"User Properties\" checkbox=\"User cannot change password\" state=\"checked\"\n"
        "1970-01-01 01:16:44 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:47 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:48 + (break)\n"
        "1970-01-01 01:16:48 - type=\"KBD_INPUT\" data=\"ABCDABCDABCD\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMeta4)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        auto send_kbd = [&]{
            meta.kbd_input(now, 'A');
            meta.kbd_input(now, 'B');
            meta.kbd_input(now, 'C');
            meta.kbd_input(now, 'D');
        };

        send_kbd(); now += 1s;

        meta.title_changed(now, "Blah1"_av); now += 1s;

        send_kbd();

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, ""_av),
            KVLog("button"_av, "Démarrer"_av),
        });
        now += 1s;

        send_kbd(); now += 1s;

        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av); now += 1s;
        send_kbd(); now += 1s;
        send_kbd(); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.next_video(now);
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:41 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:42 - type=\"BUTTON_CLICKED\" windows=\"\" button=\"Démarrer\"\n"
        "1970-01-01 01:16:44 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:47 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:48 + (break)\n"
        "1970-01-01 01:16:48 - type=\"KBD_INPUT\" data=\"ABCDABCDABCDABCDABCD\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMeta5)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);

        meta.kbd_input(now, 'A'); now += 1s;

        meta.title_changed(now, "Blah1"_av); now += 1s;

        meta.kbd_input(now, 'B');

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, ""_av),
            KVLog("button"_av, "Démarrer"_av),
        });
        now += 1s;

        meta.kbd_input(now, 'C'); now += 1s;

        meta.possible_active_window_change();

        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av); now += 1s;
        meta.kbd_input(now, 'D'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, 'E'); now += 1s;
        meta.kbd_input(now, 'F'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, 'G'); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\t'); now += 1s;
        meta.kbd_input(now, 'H'); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.next_video(now);
        meta.kbd_input(now, 'I'); now += 1s;
        meta.kbd_input(now, 'J'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'K'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah4"_av); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'a'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'L'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah5"_av); now += 1s;
        meta.kbd_input(now, 'M'); now += 1s;
        meta.kbd_input(now, 'N'); now += 1s;
        meta.kbd_input(now, 'O'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'P'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah6"_av); now += 1s;
        meta.kbd_input(now, 'Q'); now += 1s;
        meta.kbd_input(now, 'R'); now += 1s;
        meta.kbd_input(now, 0x2191); now += 1s; // UP
        meta.kbd_input(now, 'S'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'T'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, 'U'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'V'); now += 1s;
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:41 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:42 - type=\"BUTTON_CLICKED\" windows=\"\" button=\"Démarrer\"\n"
        "1970-01-01 01:16:43 - type=\"KBD_INPUT\" data=\"ABC\"\n"
        "1970-01-01 01:16:44 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:45 - type=\"KBD_INPUT\" data=\"D/<enter>\"\n"
        "1970-01-01 01:16:48 - type=\"KBD_INPUT\" data=\"EF/<enter>\"\n"
        "1970-01-01 01:16:52 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:52 - type=\"KBD_INPUT\" data=\"/<enter>G/<enter>\"\n"
        "1970-01-01 01:16:57 + (break)\n"
        "1970-01-01 01:17:00 - type=\"KBD_INPUT\" data=\"/<enter>/<tab>HIK\"\n"
        "1970-01-01 01:17:01 + type=\"TITLE_BAR\" data=\"Blah4\"\n"
        "1970-01-01 01:17:06 - type=\"KBD_INPUT\" data=\"/<backspace>/<backspace>L\"\n"
        "1970-01-01 01:17:07 + type=\"TITLE_BAR\" data=\"Blah5\"\n"
        "1970-01-01 01:17:13 - type=\"KBD_INPUT\" data=\"MP\"\n"
        "1970-01-01 01:17:14 + type=\"TITLE_BAR\" data=\"Blah6\"\n"
        "1970-01-01 01:17:27 - type=\"KBD_INPUT\" data=\"QR/<up>/<backspace>T//U//V\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionSessionLog)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, false);
        Capture::SessionLogAgent log_agent(meta, make_meta_params());

        meta.session_update(now, LogId::NEW_PROCESS, {
            KVLog("command_line"_av, "abc"_av),
        });
        now += 1s;

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, "de"_av),
            KVLog("button"_av, "fg"_av),
        });
        now += 1s;
    }

    RED_CHECK_EQ(
        trans.buf,
        "1970-01-01 01:16:40 - type=\"NEW_PROCESS\" command_line=\"abc\"\n"
        "1970-01-01 01:16:41 - type=\"BUTTON_CLICKED\" windows=\"de\" button=\"fg\"\n"
    );
}


RED_AUTO_TEST_CASE(TestSessionMetaHiddenKey)
{
    force_paris_timezone();
    BufTransport trans;

    {
        MonotonicTimePoint now{1000s};
        Capture::SessionMeta meta = make_session_meta(now, trans, true);

        meta.kbd_input(now, 'A'); now += 1s;

        meta.title_changed(now, "Blah1"_av); now += 1s;

        meta.kbd_input(now, 'B');

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, ""_av),
            KVLog("button"_av, "Démarrer"_av),
        });
        now += 1s;

        meta.kbd_input(now, 'C'); now += 1s;

        meta.possible_active_window_change();

        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah2"_av); now += 1s;
        meta.kbd_input(now, 'D'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, 'E'); now += 1s;
        meta.kbd_input(now, 'F'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, 'G'); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.title_changed(now, "Blah3"_av); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\r'); now += 1s;
        meta.kbd_input(now, '\t'); now += 1s;
        meta.kbd_input(now, 'H'); now += 1s;
        meta.periodic_snapshot(now, 0, 0);
        meta.next_video(now);
        meta.kbd_input(now, 'I'); now += 1s;
        meta.kbd_input(now, 'J'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'K'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah4"_av); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'a'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'L'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah5"_av); now += 1s;
        meta.kbd_input(now, 'M'); now += 1s;
        meta.kbd_input(now, 'N'); now += 1s;
        meta.kbd_input(now, 'O'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'P'); now += 1s;
        meta.possible_active_window_change();
        meta.title_changed(now, "Blah6"_av); now += 1s;
        meta.kbd_input(now, 'Q'); now += 1s;
        meta.kbd_input(now, 'R'); now += 1s;
        meta.kbd_input(now, 0x2191); now += 1s; // UP
        meta.kbd_input(now, 'S'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'T'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, 'U'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, '/'); now += 1s;
        meta.kbd_input(now, 0x08); now += 1s;
        meta.kbd_input(now, 'V'); now += 1s;

        meta.session_update(now, LogId::BUTTON_CLICKED, {
            KVLog("windows"_av, "\"Connexion Bureau à distance\""_av),
            KVLog("button"_av, "&Connexion"_av),
        });
        now += 1s;
    }

    RED_CHECK(
        trans.buf ==
        "1970-01-01 01:16:41 + type=\"TITLE_BAR\" data=\"Blah1\"\n"
        "1970-01-01 01:16:42 - type=\"BUTTON_CLICKED\" windows=\"\" button=\"Démarrer\"\n"
        "1970-01-01 01:16:43 - type=\"KBD_INPUT\" data=\"ABC\"\n"
        "1970-01-01 01:16:44 + type=\"TITLE_BAR\" data=\"Blah2\"\n"
        "1970-01-01 01:16:45 - type=\"KBD_INPUT\" data=\"D\"\n"
        "1970-01-01 01:16:48 - type=\"KBD_INPUT\" data=\"EF\"\n"
        "1970-01-01 01:16:52 + type=\"TITLE_BAR\" data=\"Blah3\"\n"
        "1970-01-01 01:16:52 - type=\"KBD_INPUT\" data=\"G\"\n"
        "1970-01-01 01:16:57 + (break)\n"
        "1970-01-01 01:17:00 - type=\"KBD_INPUT\" data=\"HIK\"\n"
        "1970-01-01 01:17:01 + type=\"TITLE_BAR\" data=\"Blah4\"\n"
        "1970-01-01 01:17:06 - type=\"KBD_INPUT\" data=\"L\"\n"
        "1970-01-01 01:17:07 + type=\"TITLE_BAR\" data=\"Blah5\"\n"
        "1970-01-01 01:17:13 - type=\"KBD_INPUT\" data=\"MP\"\n"
        "1970-01-01 01:17:14 + type=\"TITLE_BAR\" data=\"Blah6\"\n"
        "1970-01-01 01:17:28 - type=\"BUTTON_CLICKED\" windows=\"\\\"Connexion Bureau à distance\\\"\" button=\"&Connexion\"\n"
        "1970-01-01 01:17:28 - type=\"KBD_INPUT\" data=\"QRT/U/V\"\n"
        ""_av
    );
}

namespace
{
    struct TestGraphicToFile
    {
        MonotonicTimePoint now{1000s};

        BmpCache bmp_cache;
        GlyphCache gly_cache;
        std::array<RdpPointer, PointerCache::MAX_POINTER_COUNT> pointers;
        RDPDrawable drawable;
        GraphicToFile consumer;

        TestGraphicToFile(SequencedTransport& trans, Rect scr, bool small_cache)
        : bmp_cache(
            BmpCache::Recorder, BitsPerPixel{24}, 3, false,
            BmpCache::CacheOption(small_cache ? 2 : 600, 256, false),
            BmpCache::CacheOption(small_cache ? 2 : 300, 1024, false),
            BmpCache::CacheOption(small_cache ? 2 : 262, 4096, false),
            BmpCache::CacheOption(),
            BmpCache::CacheOption(),
            BmpCache::Verbose::none
        )
        , drawable(scr.cx, scr.cy)
        , consumer(now, RealTimePoint{now.time_since_epoch()},
            trans, BitsPerPixel{24}, false, Rect(), bmp_cache, gly_cache,
            PointerCache::SourcePointersView{pointers},
            drawable.impl(), WrmCompressionAlgorithm::no_compression,
            RDPSerializerVerbose::none)
        {}

        void next_second(int n = 1) /*NOLINT*/
        {
            now += std::chrono::seconds(n);
            consumer.timestamp(now);
        }
    };
} // namespace


const char expected_stripped_wrm[] =
/* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
           "\x03\x00\x20\x03\x58\x02\x18\x00" // WRM version = 3, width = 800, height=600, bpp=24
           "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"
           //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

// initial screen content PNG image
/* 0000 */ "\x00\x10\xcc\x05\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x03\x20\x00\x00\x02\x58\x08\x02\x00\x00\x00\x15\x14\x15" //... ...X........
/* 0020 */ "\x27\x00\x00\x05\x8b\x49\x44\x41\x54\x78\x9c\xed\xc1\x01\x0d\x00" //'....IDATx......
/* 0030 */ "\x00\x00\xc2\xa0\xf7\x4f\x6d\x0e\x37\xa0\x00\x00\x00\x00\x00\x00" //.....Om.7.......
/* 0040 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0060 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0070 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0080 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0090 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0100 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0110 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0120 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0130 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0140 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0150 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0160 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0170 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0180 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0190 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0200 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0210 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0220 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0230 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0240 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0250 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0260 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0270 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0280 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0290 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0300 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0310 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0320 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0330 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0340 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0350 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0360 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0370 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0380 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0390 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0400 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0410 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0420 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0430 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0440 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0450 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0460 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0470 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0480 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0490 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0500 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0510 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0520 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0530 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0540 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0550 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0560 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0570 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0580 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0590 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 05a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x57\x03" //..............W.
/* 05b0 */ "\xfc\x93\x00\x01\x4b\x66\x2c\x0e\x00\x00\x00\x00\x49\x45\x4e\x44" //....Kf,.....IEND
/* 05c0 */ "\xae\x42\x60\x82"                                                 //.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

           "\x00\x00\x10\x00\x00\x00\x01\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
/* 0000 */ "\x09\x0a\x2c\x20\x03\x58\x02\xff"         // Green Rect(0, 0, 800, 600)

/* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x40\x42\x0F\x00\x00\x00\x00\x00" // 0x0F4240 = 1000000
           "\x00\x00\x00\x00\x00"

/* 0000 */ "\x00\x00\x12\x00\x00\x00\x01\x00" // 0000: ORDERS  0012:chunk_len=18 0002: 1 orders
           "\x01\x6e\x32\x00\xbc\x02\x1e\x00\x00\xff"  // Blue  Rect(0, 50, 700, 80)

/* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x00\x09\x3d\x00\x00\x00\x00\x00" // time = 4000000
           "\x00\x00\x00\x00\x00"

/* 0000 */ "\x00\x00\x0d\x00\x00\x00\x01\x00" // 0000: ORDERS
           "\x11\x32\x32\xff\xff"

/* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP
           "\x80\x8d\x5b\x00\x00\x00\x00\x00" // time = 6000000
           "\x00\x00\x00\x00\x00"

/* 0000 */ "\x00\x00\x0d\x00\x00\x00\x01\x00" // 0000: ORDERS
           "\x11\x62\x32\x00\x00"

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
/* 0000 */ "\xc0\xcf\x6a\x00\x00\x00\x00\x00" // time =
           "\x00\x00\x00\x00\x00"
    ;

RED_AUTO_TEST_CASE(Test6SecondsStrippedScreenToWrm)
{
    force_paris_timezone();
    Rect screen_rect(0, 0, 800, 600);
    CheckTransport trans(cstr_array_view(expected_stripped_wrm));
    TestGraphicToFile tgtf(trans, screen_rect, false);
    GraphicToFile& consumer = tgtf.consumer;

    auto const color_ctx = gdi::ColorCtx::depth24();

    consumer.draw(RDPOpaqueRect(screen_rect, encode_color24()(NamedBGRColor::GREEN)), screen_rect, color_ctx);
    tgtf.next_second();
    consumer.send_timestamp_chunk();

    consumer.draw(RDPOpaqueRect(Rect(0, 50, 700, 30), encode_color24()(NamedBGRColor::BLUE)), screen_rect, color_ctx);
    consumer.sync();
    tgtf.next_second();
    tgtf.next_second();
    tgtf.next_second();
    consumer.send_timestamp_chunk();

    consumer.draw(RDPOpaqueRect(Rect(0, 100, 700, 30), encode_color24()(NamedBGRColor::WHITE)), screen_rect, color_ctx);
    tgtf.next_second();
    tgtf.next_second();
    consumer.send_timestamp_chunk();

    consumer.draw(RDPOpaqueRect(Rect(0, 150, 700, 30), encode_color24()(NamedBGRColor::RED)), screen_rect, color_ctx);
    tgtf.next_second();

    consumer.send_timestamp_chunk();
    consumer.sync();
}

const char expected_stripped_wrm2[] =
/* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
           "\x03\x00\x20\x03\x58\x02\x18\x00" // WRM version = 3, width = 800, height=600, bpp=24
           "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"
           //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

// initial screen content PNG image
/* 0000 */ "\x00\x10\xcc\x05\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x03\x20\x00\x00\x02\x58\x08\x02\x00\x00\x00\x15\x14\x15" //... ...X........
/* 0020 */ "\x27\x00\x00\x05\x8b\x49\x44\x41\x54\x78\x9c\xed\xc1\x01\x0d\x00" //'....IDATx......
/* 0030 */ "\x00\x00\xc2\xa0\xf7\x4f\x6d\x0e\x37\xa0\x00\x00\x00\x00\x00\x00" //.....Om.7.......
/* 0040 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0060 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0070 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0080 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0090 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0100 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0110 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0120 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0130 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0140 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0150 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0160 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0170 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0180 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0190 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0200 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0210 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0220 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0230 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0240 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0250 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0260 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0270 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0280 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0290 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 02f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0300 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0310 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0320 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0330 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0340 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0350 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0360 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0370 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0380 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0390 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 03f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0400 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0410 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0420 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0430 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0440 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0450 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0460 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0470 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0480 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0490 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 04f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0500 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0510 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0520 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0530 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0540 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0550 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0560 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0570 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0580 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0590 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 05a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x57\x03" //..............W.
/* 05b0 */ "\xfc\x93\x00\x01\x4b\x66\x2c\x0e\x00\x00\x00\x00\x49\x45\x4e\x44" //....Kf,.....IEND
/* 05c0 */ "\xae\x42\x60\x82"                                                 //.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

           "\x00\x00\x1A\x00\x00\x00\x02\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
/* 0000 */ "\x09\x0a\x2c\x20\x03\x58\x02\xff"         // Green Rect(0, 0, 800, 600)
           "\x01\x6e\x32\x00\xbc\x02\x1e\x00\x00\xff"  // Blue  Rect(0, 50, 700, 80)

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x40\x42\x0F\x00\x00\x00\x00\x00" // 0x0F4240 = 1000000
           "\x00\x00\x00\x00\x00"

           "\x00\x00\x12\x00\x00\x00\x02\x00"
/* 0000 */ "\x11\x32\x32\xff\xff"             // encode_color24()(NamedBGRColor::WHITE) rect
           "\x11\x62\x32\x00\x00"             // encode_color24()(NamedBGRColor::RED) rect

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
/* 0000 */ "\xc0\xcf\x6a\x00\x00\x00\x00\x00" // time 1007000000
           "\x00\x00\x00\x00\x00"

           "\xf0\x03\x15\x00\x00\x00\x01\x00"
/* 0000 */ "\xc0\xcf\x6a\x00\x00\x00\x00\x00" // time 1007000000
           "\x00\x00\x00\x00\x00"

           "\x00\x00\x13\x00\x00\x00\x01\x00"
/* 0000 */ "\x01\x1f\x05\x00\x05\x00\x0a\x00\x0a\x00\x00" // encode_color24()(NamedBGRColor::BLACK) rect
   ;


RED_AUTO_TEST_CASE(Test6SecondsStrippedScreenToWrmReplay2)
{
    force_paris_timezone();
    Rect screen_rect(0, 0, 800, 600);
    CheckTransport trans(cstr_array_view(expected_stripped_wrm2));
    TestGraphicToFile tgtf(trans, screen_rect, false);
    GraphicToFile& consumer = tgtf.consumer;

    auto const color_ctx = gdi::ColorCtx::depth24();

    consumer.draw(RDPOpaqueRect(screen_rect, encode_color24()(NamedBGRColor::GREEN)), screen_rect, color_ctx);
    consumer.draw(RDPOpaqueRect(Rect(0, 50, 700, 30), encode_color24()(NamedBGRColor::BLUE)), screen_rect, color_ctx);
    tgtf.next_second();
    consumer.send_timestamp_chunk();

    consumer.draw(RDPOpaqueRect(Rect(0, 100, 700, 30), encode_color24()(NamedBGRColor::WHITE)), screen_rect, color_ctx);
    consumer.draw(RDPOpaqueRect(Rect(0, 150, 700, 30), encode_color24()(NamedBGRColor::RED)), screen_rect, color_ctx);
    tgtf.next_second(6);
    consumer.send_timestamp_chunk();

    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 10), encode_color24()(NamedBGRColor::BLACK)), screen_rect, color_ctx);

    consumer.send_timestamp_chunk();
    consumer.sync();
}


RED_AUTO_TEST_CASE(TestCaptureToWrmReplayToPng)
{
    force_paris_timezone();
    Rect screen_rect(0, 0, 800, 600);

    BufTransport trans;
    TestGraphicToFile tgtf(trans, screen_rect, false);
    GraphicToFile& consumer = tgtf.consumer;

    auto const color_ctx = gdi::ColorCtx::depth24();

    RDPOpaqueRect cmd0(screen_rect, encode_color24()(NamedBGRColor::GREEN));
    consumer.draw(cmd0, screen_rect, color_ctx);
    RDPOpaqueRect cmd1(Rect(0, 50, 700, 30), encode_color24()(NamedBGRColor::BLUE));
    consumer.draw(cmd1, screen_rect, color_ctx);
    tgtf.next_second();
    consumer.sync();

    RDPOpaqueRect cmd2(Rect(0, 100, 700, 30), encode_color24()(NamedBGRColor::WHITE));
    consumer.draw(cmd2, screen_rect, color_ctx);
    RDPOpaqueRect cmd3(Rect(0, 150, 700, 30), encode_color24()(NamedBGRColor::RED));
    consumer.draw(cmd3, screen_rect, color_ctx);
    tgtf.next_second(6);
    consumer.sync();

    RED_TEST_PASSPOINT();
    RED_TEST(trans.size() == 1580);

    GeneratorTransport in_wrm_trans(trans.data());

    FileToGraphic player(in_wrm_trans, false, FileToGraphic::Verbose(0));
    RDPDrawable drawable(player.get_wrm_info().width, player.get_wrm_info().height);
    player.add_consumer(&drawable, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/wrm_to_png/1.png");

    // Timestamp
    RED_TEST(player.next_order());

    // Green Rect
    player.interpret_order();
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/wrm_to_png/2.png");
    RED_TEST(player.next_order());

    // Blue Rect
    player.interpret_order();
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/wrm_to_png/3.png");
    RED_TEST(player.next_order());

    // White Rect
    player.interpret_order();
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/wrm_to_png/4.png");
    RED_TEST(player.next_order());

    // Red Rect
    player.interpret_order();
    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/wrm_to_png/5.png");
    RED_TEST(!player.next_order());
}


const char expected_Red_on_Blue_wrm[] =
/* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
           "\x03\x00\x64\x00\x64\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
           "\x02\x00\x00\x01\x02\x00\x00\x04\x02\x00\x00\x10"  // caches sizes
           //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

/* 0000 */ "\x00\x10\x75\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x64\x00\x00\x00\x64\x08\x02\x00\x00\x00\xff\x80\x02" //...d...d........
/* 0020 */ "\x03\x00\x00\x00\x34\x49\x44\x41\x54\x78\x9c\xed\xc1\x01\x0d\x00" //....4IDATx......
/* 0030 */ "\x00\x00\xc2\xa0\xf7\x4f\x6d\x0e\x37\xa0\x00\x00\x00\x00\x00\x00" //.....Om.7.......
/* 0040 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x7e\x0c\x75\x94\x00\x01\xa8\x50\xf2" //.......~.u....P.
/* 0060 */ "\x39\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82"             //9....IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

/* 0000 */ "\x00\x00\x2d\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
/* 0000 */ "\x19\x0a\x4c\x64\x64\xff"         // Blue rect  // order 0A=opaque rect
// -----------------------------------------------------
/* 0020 */ "\x03\x09\x00\x00\x04\x02"         // Secondary drawing order header. Order = 02: Compressed bitmap
           "\x01\x00\x14\x0a\x18\x07\x00\x00\x00" // 0x01=cacheId 0x00=pad 0x14=width(20) 0x0A=height(10) 0x18=24 bits
                                                  // 0x0007=bitmapLength 0x0000=cacheIndex
           "\xc0\x04\x00\x00\xff\x00\x94"         // compressed bitamp data (7 bytes)
// -----------------------------------------------------

           "\x59\x0d\x3d\x01\x00\x5a\x14\x0a\xcc" // order=0d : MEMBLT

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x40\x42\x0F\x00\x00\x00\x00\x00" // 0x0F4240 = 1000000
           "\x00\x00\x00\x00\x00"

           "\x00\x00\x1e\x00\x00\x00\x01\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
// -----------------------------------------------------
/* 0000 */ "\x03\x09\x00\x00\x04\x02"
           "\x01\x00\x14\x0a\x18\x07\x00\x00\x00"
           "\xc0\x04\x00\x00\xff\x00\x94"
// -----------------------------------------------------
           ;

RED_AUTO_TEST_CASE(TestSaveCache)
{
    force_paris_timezone();
    Rect scr(0, 0, 100, 100);
    CheckTransport trans(cstr_array_view(expected_Red_on_Blue_wrm));
    trans.disable_remaining_error();
    TestGraphicToFile tgtf(trans, scr, true);
    GraphicToFile& consumer = tgtf.consumer;

    auto const color_ctx = gdi::ColorCtx::depth24();

    consumer.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::BLUE)), scr, color_ctx);

    uint8_t comp20x10RED[] = {
        0xc0, 0x04, 0x00, 0x00, 0xFF, // MIX 20 (0, 0, FF)
        0x00, 0x94                    // FILL 9 * 20
    };

    Bitmap bloc20x10(BitsPerPixel{24}, BitsPerPixel{24}, nullptr, 20, 10, comp20x10RED, sizeof(comp20x10RED), true );
    consumer.draw(
        RDPMemBlt(0, Rect(0, scr.cy - 10, bloc20x10.cx(), bloc20x10.cy()), 0xCC, 0, 0, 0),
        scr,
        bloc20x10);
    consumer.sync();

    tgtf.next_second();

    consumer.save_bmp_caches();
    consumer.sync();
}

const char expected_reset_rect_wrm[] =
/* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
           "\x03\x00\x64\x00\x64\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
           "\x02\x00\x00\x01\x02\x00\x00\x04\x02\x00\x00\x10"  // caches sizes
           //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

/* 0000 */ "\x00\x10\x75\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x64\x00\x00\x00\x64\x08\x02\x00\x00\x00\xff\x80\x02" //...d...d........
/* 0020 */ "\x03\x00\x00\x00\x34\x49\x44\x41\x54\x78\x9c\xed\xc1\x01\x0d\x00" //....4IDATx......
/* 0030 */ "\x00\x00\xc2\xa0\xf7\x4f\x6d\x0e\x37\xa0\x00\x00\x00\x00\x00\x00" //.....Om.7.......
/* 0040 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x7e\x0c\x75\x94\x00\x01\xa8\x50\xf2" //.......~.u....P.
/* 0060 */ "\x39\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82"             //9....IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

/* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
           "\x19\x0a\x1c\x64\x64\xff\x11"     // Red Rect
           "\x5f\x05\x05\xf6\xf6\x00\xff"     // Blue Rect
           "\x11\x5f\x05\x05\xf6\xf6\xff\x00" // Red Rect

           // save orders cache
/* 0000 */ "\x02\x10"
/* 0000 */ "\x0F\x02"                         //.data length
/* 0000 */ "\x00\x00\x01\x00"

/* 0000 */ "\x0a\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0010 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0020 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0030 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x0a\x00\x50" //...............P
/* 0040 */ "\x00\x50\x00\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //.P..............
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0060 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0070 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0080 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0090 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00" //................
/* 00a0 */ "\x01\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00" //................
/* 00b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0100 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0110 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0120 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0130 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0140 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0150 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0160 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0170 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0180 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0190 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 01c0 */ "\x00\x00\x00\x00\x00\x00"                                         //......

/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // MultiDstBlt

/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // MultiOpaqueRect

/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // MultiOpaqueRect
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // MultiScrBlt

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x40\x42\x0F\x00\x00\x00\x00\x00" // 0x0F4240 = 1000000
           "\x00\x00\x00\x00\x00"

           "\x00\x00\x10\x00\x00\x00\x01\x00"
           "\x11\x3f\x0a\x0a\xec\xec\x00\xff" // Green Rect
           ;

RED_AUTO_TEST_CASE(TestSaveOrderStates)
{
    force_paris_timezone();
    Rect scr(0, 0, 100, 100);
    CheckTransport trans(cstr_array_view(expected_reset_rect_wrm));
    TestGraphicToFile tgtf(trans, scr, true);
    GraphicToFile& consumer = tgtf.consumer;

    auto const color_cxt = gdi::ColorCtx::depth24();

    consumer.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(scr.shrink(5), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(scr.shrink(10), encode_color24()(NamedBGRColor::RED)), scr, color_cxt);

    consumer.sync();

    consumer.send_save_state_chunk();

    tgtf.next_second();
    consumer.send_timestamp_chunk();
    consumer.draw(RDPOpaqueRect(scr.shrink(20), encode_color24()(NamedBGRColor::GREEN)), scr, color_cxt);

    consumer.sync();
}

const char expected_continuation_wrm[] =
/* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
           "\x01\x00\x64\x00\x64\x00\x18\x00" // WRM version 1, width = 20, height=10, bpp=24
           "\x02\x00\x00\x01\x02\x00\x00\x04\x02\x00\x00\x10"  // caches sizes

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

           "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
           "\x40\x42\x0F\x00\x00\x00\x00\x00" // 0x0F4240 = 1000000
           "\x00\x00\x00\x00\x00"

           // save images
/* 0000 */ "\x00\x10\x49\x01\x00\x00\x01\x00"

/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x64\x00\x00\x00\x64\x08\x02\x00\x00\x00\xff\x80\x02" //...d...d........
/* 0020 */ "\x03\x00\x00\x01\x08\x49\x44\x41\x54\x78\x9c\xed\xdd\x31\x0e\x83" //.....IDATx...1..
/* 0030 */ "\x30\x10\x00\x41\x3b\xe2\xff\x5f\x86\x32\xc8\x05\xb0\x55\x14\x34" //0..A;.._.2...U.4
/* 0040 */ "\xd3\xb9\xb3\x56\xa7\x73\x07\x73\x1f\x3c\xf5\xf9\xf5\x05\xfe\x89" //...V.s.s.<......
/* 0050 */ "\x58\x81\x58\x81\x58\xc1\xb6\x9c\xe7\xb0\xf1\xbf\xf6\x31\xcf\x47" //X.X.X........1.G
/* 0060 */ "\x93\x15\x88\x15\x88\x15\x88\x15\xac\x0b\x7e\xb1\x6c\xb8\xd7\xbb" //..........~.l...
/* 0070 */ "\x7e\xdf\x4c\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" //~.LV V V V V V V
/* 0080 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 0090 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00a0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00b0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00c0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00d0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00e0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 00f0 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 0100 */ "\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56\x20\x56" // V V V V V V V V
/* 0110 */ "\x70\xf3\x3d\x78\xff\xff\x38\x33\x59\x81\x58\x81\x58\x81\x58\xc1" //p.=x..83Y.X.X.X.
/* 0120 */ "\xb4\xc0\x9f\x33\x59\x81\x58\x81\x58\x81\x58\xc1\x01\x8e\xa9\x07" //...3Y.X.X.X.....
/* 0130 */ "\xcb\xdb\x96\x4d\x96\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60" //...M.....IEND.B`
/* 0140 */ "\x82"

           // save orders cache
           "\x02\x10\xA0\x01\x00\x00\x01\x00"
/* 0000 */ "\x0a\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0010 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0020 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0030 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

/* 0000 */ "\x0a\x00\x0a\x00\x50\x00\x50\x00\xff\x00\x00\x00\x00\x00\x00\x00" //....P.P.........
/* 0010 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0020 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0030 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0040 */ "\x01\x00\x01\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00\x00" //................
/* 0050 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0060 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0070 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0080 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0090 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00a0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00b0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00c0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00d0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00e0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 00f0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0100 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0110 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0120 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0130 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0140 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................
/* 0150 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

           "\x00\x00\x10\x00\x00\x00\x01\x00"
           "\x11\x3f\x0a\x0a\xec\xec\x00\xff" // Green Rect
           ;

RED_AUTO_TEST_CASE(TestImageChunk)
{
    force_paris_timezone();
    const char expected_stripped_wrm[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version = 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"
               //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // encode_color24()(NamedBGRColor::RED) rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // encode_color24()(NamedBGRColor::BLUE) RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // encode_color24()(NamedBGRColor::WHITE) RECT

    /* 0000 */ "\x00\x10\x74\x00\x00\x00\x01\x00" // 0x1000: IMAGE_CHUNK 0048: chunk_len=86 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
        "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00"             //.............
        "\x3b\x37\xe9\xb1"                                                 //;7..
        "\x00\x00\x00"
/* 0000 */ "\x33\x49\x44\x41\x54\x28\x91\x63\x64\x60\xf8\xcf\x80\x1b\xfc\xff" //3IDAT(.cd`......
/* 0010 */ "\xcf\xc0\xc8\x88\x53\x96\x09\x8f\x4e\x82\x60\x88\x6a\x66\x41\xe3" //....S...N.`.jfA.
/* 0020 */ "\xff\x67\x40\x0b\x9f\xff\xc8\x22\x8c\xa8\xa1\x3b\x70\xce\x66\x1c" //.g@...."...;p.f.
/* 0030 */ "\xb0\x78\x06\x00\x69\xde\x0a\x12\x3d\x77\xd0\x9e\x00\x00\x00\x00" //.x..i...=w......
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`
    ;

    Rect scr(0, 0, 20, 10);
    CheckTransport trans(cstr_array_view(expected_stripped_wrm));
    TestGraphicToFile tgtf(trans, scr, false);
    GraphicToFile& consumer = tgtf.consumer;
    RDPDrawable& drawable = tgtf.drawable;

    auto const color_cxt = gdi::ColorCtx::depth24();
    drawable.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.sync();
    consumer.send_image_chunk();
}

RED_AUTO_TEST_CASE(TestImagePNGMediumChunks)
{
    force_paris_timezone();
    // Same test as above but forcing use of small png chunks
    // Easier to do than write tests with huge pngs to force PNG chunking.

    const char expected[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"
               //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // encode_color24()(NamedBGRColor::RED) rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // encode_color24()(NamedBGRColor::BLUE) RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // encode_color24()(NamedBGRColor::WHITE) RECT

    /* 0000 */ "\x01\x10\x64\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order

        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
        "\x08\x02\x00\x00\x00"             //.............
        "\x3b\x37\xe9\xb1"                                                 //;7..
        "\x00\x00\x00\x32\x49\x44\x41\x54"                                 //...2IDAT
        "\x28\x91\x63\xfc\xcf\x80\x17"
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
        "\x09" //(.c..........K..
        "\xbf\x5e\xfc\x60\x88\x6a\x66\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
        "\x54" //.^.`.jfA.32....T
        "\x91\xff\x0c\x28\x81\x37\x70\xce\x66\x1c\xb0\x78\x06\x00\x69\xdc" //...(.7p.f..x..i.
        "\x0a\x12"                                                         //..
        "\x86"
        "\x00\x10\x17\x00\x00\x00\x01\x00"  // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x4a\x0c\x44"                                                 //.J.D
        "\x00\x00\x00\x00\x49\x45\x4e\x44"                             //....IEND
        "\xae\x42\x60\x82"                                             //.B`.
        ;

    Rect scr(0, 0, 20, 10);
    CheckTransport trans(cstr_array_view(expected));
    TestGraphicToFile tgtf(trans, scr, false);
    GraphicToFile& consumer = tgtf.consumer;
    RDPDrawable& drawable = tgtf.drawable;

    auto const color_cxt = gdi::ColorCtx::depth24();
    drawable.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.sync();

    OutChunkedBufferingTransport<100> png_trans(trans);
    RED_CHECK_NO_THROW(consumer.dump_png24(png_trans, true));
}

RED_AUTO_TEST_CASE(TestImagePNGSmallChunks)
{
    // Same test as above but forcing use of small png chunks
    // Easier to do than write tests with huge pngs to force PNG chunking.

    force_paris_timezone();
    const char expected[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=28 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version = 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"
               //"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // For WRM version >3

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // encode_color24()(NamedBGRColor::RED) rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // encode_color24()(NamedBGRColor::BLUE) RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // encode_color24()(NamedBGRColor::WHITE) RECT

    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ;

    Rect scr(0, 0, 20, 10);
    CheckTransport trans(cstr_array_view(expected));
    TestGraphicToFile tgtf(trans, scr, false);
    GraphicToFile& consumer = tgtf.consumer;
    RDPDrawable& drawable = tgtf.drawable;

    auto const color_cxt = gdi::ColorCtx::depth24();
    drawable.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(scr, encode_color24()(NamedBGRColor::RED)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), encode_color24()(NamedBGRColor::BLUE)), scr, color_cxt);
    drawable.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), encode_color24()(NamedBGRColor::WHITE)), scr, color_cxt);
    consumer.sync();

    OutChunkedBufferingTransport<16> png_trans(trans);
    consumer.dump_png24(png_trans, true);
    // drawable.dump_png24(png_trans, true); true);
}

const char source_wrm_png[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24 PAD: 2 bytes
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

    /* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00" // = 0
           "\x00\x00\x00\x00\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ;

   const char source_wrm_png_then_other_chunk[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

           "\xf4\x03\x18\x00\x00\x00\x01\x00" // 03F4: TIMES 0010: chunk_len=24 0001: 1 order
/* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00"
           "\x00\xca\x9a\x3b\x00\x00\x00\x00" // 0x3B9ACA00 = 1000000000

    /* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\x00\x00\x00\x00\x00\x00\x00" // = 0
           "\x00\x00\x00\x00\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
    /* 0000 */ "\xf0\x03\x15\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\x09\x3d\x00\x00\x00\x00\x00" // time = 4000000
           "\x00\x00\x00\x00\x00"
       ;

RED_AUTO_TEST_CASE(TestReload)
{
    force_paris_timezone();
    struct Test
    {
        char const* name;
        chars_view data;
        unsigned file_len;
        time_t monotonic_time;
        time_t real_time;
    };

    RED_TEST_CONTEXT_DATA(auto const& test, test.name, {
        Test{"TestReloadSaveCache", cstr_array_view(expected_Red_on_Blue_wrm), 298, 1, 1001},
        Test{"TestReloadOrderStates", cstr_array_view(expected_reset_rect_wrm), 341, 1, 1001},
        Test{"TestContinuationOrderStates", cstr_array_view(expected_continuation_wrm), 341, 1, 1001},
        Test{"testimg", cstr_array_view(source_wrm_png), 107, 0, 1000},
        Test{"testimg_then_other_chunk", cstr_array_view(source_wrm_png_then_other_chunk), 107, 4, 1004}
    })
    {
        BufTransport trans;

        {
            GeneratorTransport in_wrm_trans(test.data);
            FileToGraphic player(in_wrm_trans, false, FileToGraphic::Verbose(0));
            RDPDrawable drawable(player.get_wrm_info().width, player.get_wrm_info().height);
            player.add_consumer(&drawable, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
            while (player.next_order()){
                player.interpret_order();
            }
            ::dump_png24(trans, drawable, true);
            RED_CHECK(test.real_time == to_time_t(player.get_real_time()));
            RED_CHECK(test.monotonic_time == to_time_t(player.get_monotonic_time()));
        }

        RED_TEST(trans.size() == test.file_len);
    }
}

RED_AUTO_TEST_CASE(TestUtf8KbdBuffer)
{
    force_paris_timezone();
    Utf8KbdBuffer buffer;
    constexpr auto str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_av;
    static_assert(str.size() == 62);

    buffer.push(str);
    RED_TEST(buffer.last_chars(str.size()) == str);
    RED_TEST(buffer.last_chars(str.size() * 2) == str);
    RED_TEST(buffer.last_chars(str.size() / 2) == str.drop_front(str.size() / 2));

    buffer.push(str);
    RED_TEST(buffer.last_chars(str.size()) == str);
    RED_TEST(buffer.last_chars(str.size() * 2) == str_concat(str, str));
    RED_TEST(buffer.last_chars(str.size() * 3) == str_concat(str, str));
    RED_TEST(buffer.last_chars(str.size() / 2) == str.drop_front(str.size() / 2));

    buffer.push("+=/*"_av);
    RED_TEST(buffer.last_chars(128) == str_concat(str, str, "+=/*"_av));
    RED_TEST(buffer.last_chars(129) == str_concat(str, str, "+=/*"_av));

    buffer.push("a"_av);
    RED_TEST(buffer.last_chars(10) == "56789+=/*a"_av);
}

RED_AUTO_TEST_CASE(TestKbdCapture)
{
    force_paris_timezone();
    SessionLogTest report_message;

    MonotonicTimePoint const time {};
    Capture::SessionLogKbd kbd_capture(report_message);

    {
        kbd_capture.kbd_input(time, 'a');
        // flush report buffer then empty buffer
        kbd_capture.possible_active_window_change();

        RED_CHECK(report_message.events() == "KBD_INPUT data=\"a\"\n"_av);
    }

    kbd_capture.enable_kbd_input_mask(true);

    {
        kbd_capture.kbd_input(time, 'a');
        kbd_capture.possible_active_window_change();

        // prob is not enabled
        RED_CHECK(report_message.events() == ""_av);
    }

    kbd_capture.enable_kbd_input_mask(false);

    {
        kbd_capture.kbd_input(time, 'a');

        RED_CHECK(report_message.events() == ""_av);

        kbd_capture.enable_kbd_input_mask(true);

        RED_CHECK(report_message.events() == "KBD_INPUT data=\"a\"\n"_av);

        kbd_capture.kbd_input(time, 'a');
        kbd_capture.possible_active_window_change();

        RED_CHECK(report_message.events() == ""_av);
    }
}

RED_AUTO_TEST_CASE(TestKbdCapture2)
{
    force_paris_timezone();
    SessionLogTest report_message;

    MonotonicTimePoint now{};
    Capture::SessionLogKbd kbd_capture(report_message);

    {
        kbd_capture.kbd_input(now, 't');

        kbd_capture.session_update(now, LogId::INPUT_LANGUAGE, {
            KVLog("identifier"_av, "fr"_av),
            KVLog("display_name"_av, "xy\\z"_av),
        });

        RED_CHECK(report_message.events() == ""_av);

        kbd_capture.kbd_input(now, 'o');

        kbd_capture.kbd_input(now, 't');

        kbd_capture.kbd_input(now, 'o');

        kbd_capture.possible_active_window_change();

        RED_CHECK(report_message.events() == "KBD_INPUT data=\"toto\"\n"_av);
    }
}

RED_AUTO_TEST_CASE(TestKbdCapturePatternNotify)
{
    force_paris_timezone();
    SessionLogTest report_message;

    CapturePattern cap_pattern{
        CapturePattern::Filters{.is_ocr = false, .is_kbd = true},
        CapturePattern::PatternType::reg,
        "abcd"_av
    };
    Capture::PatternKbd kbd_capture(report_message, {&cap_pattern, 1}, {});

    unsigned pattern_count = 0;
    for (auto c : bytes_view("abcdaaaaaaaaaaaaaaaabcdeaabcdeaaaaaaaaaaaaabcde"_av)) {
        if (!kbd_capture.kbd_input(MonotonicTimePoint{}, c)) {
            ++pattern_count;
        }
    }

    RED_CHECK(pattern_count == 4);
    RED_CHECK(report_message.events() ==
        "KILL_PATTERN_DETECTED pattern=\"$kbd:abcd\" data=\"abcd\"\n"
        "FINDPATTERN_KILL: $kbd:abcd\x02""abcd\n"
        "KILL_PATTERN_DETECTED pattern=\"$kbd:abcd\" data=\"abcd\"\n"
        "FINDPATTERN_KILL: $kbd:abcd\x02""abcd\n"
        "KILL_PATTERN_DETECTED pattern=\"$kbd:abcd\" data=\"abcd\"\n"
        "FINDPATTERN_KILL: $kbd:abcd\x02""abcd\n"
        "KILL_PATTERN_DETECTED pattern=\"$kbd:abcd\" data=\"abcd\"\n"
        "FINDPATTERN_KILL: $kbd:abcd\x02""abcd\n"
        ""_av
    );
}


RED_AUTO_TEST_CASE(TestKbdCapturePatternKill)
{
    force_paris_timezone();
    SessionLogTest report_message;

    CapturePattern cap_pattern{
        CapturePattern::Filters{.is_ocr = false, .is_kbd = true},
        CapturePattern::PatternType::reg,
        "ab/cd"_av
    };
    Capture::PatternKbd kbd_capture(report_message, {&cap_pattern, 1}, {});

    unsigned pattern_count = 0;
    for (auto c : bytes_view("abcdab/cdaa"_av)) {
        pattern_count += !kbd_capture.kbd_input(MonotonicTimePoint{}, c);
    }
    RED_CHECK_EQUAL(1, pattern_count);
    RED_CHECK(report_message.events() ==
        "KILL_PATTERN_DETECTED pattern=\"$kbd:ab/cd\" data=\"ab/cd\"\n"
        "FINDPATTERN_KILL: $kbd:ab/cd\x02""ab/cd\n"
        ""_av);

    // new line (\r because windows layout)
    // -> reset buffer position (ignore "aa" of previous line)
    for (auto c : bytes_view("aa\rab/cd"_av)) {
        pattern_count += !kbd_capture.kbd_input(MonotonicTimePoint{}, c);
    }
    RED_CHECK_EQUAL(2, pattern_count);
    RED_CHECK(report_message.events() ==
        "KILL_PATTERN_DETECTED pattern=\"$kbd:ab/cd\" data=\"ab/cd\"\n"
        "FINDPATTERN_KILL: $kbd:ab/cd\x02""ab/cd\n"
        ""_av);
}

RED_AUTO_TEST_CASE(TestKbdEnableWithoutPattern)
{
    force_paris_timezone();
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    test_capture_context("resizing-capture-1", CaptureFlags::ocr,
        800, 600, record_wd, hash_wd,
        KbdLogParams{true, false, false},
        [](Capture& capture, Rect /*scr*/) {
            MonotonicTimePoint now{1000s};
            capture.kbd_input(now, 95);
        }
    );

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}

RED_AUTO_TEST_CASE(TestSample0WRM)
{
    force_paris_timezone();
    int fd = ::open(FIXTURES_PATH "/sample0.wrm", O_RDONLY);
    RED_REQUIRE_NE(fd, -1);

    InFileTransport in_wrm_trans(unique_fd{fd});
    FileToGraphic player(in_wrm_trans, false, FileToGraphic::Verbose(0));

    auto const& info = player.get_wrm_info();
    RDPDrawable drawable(info.width, info.height);
    player.add_consumer(&drawable, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    bool requested_to_stop = false;

    RED_CHECK(1352304810 == to_time_t(player.get_monotonic_time()));
    player.play(requested_to_stop, MonotonicTimePoint(), MonotonicTimePoint::max());

    RED_CHECK_IMG(drawable, IMG_TEST_PATH "/sample0.png");

    RED_CHECK(1352304870 == to_time_t(player.get_monotonic_time()));
}

RED_AUTO_TEST_CASE(TestReadPNGFromChunkedTransport)
{
    force_paris_timezone();
    auto source_png =
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ""_av
    ;

    InStream stream(source_png);

    uint16_t chunk_type = stream.in_uint16_le();
    uint32_t chunk_size = stream.in_uint32_le();
    uint16_t chunk_count = stream.in_uint16_le();
    (void)chunk_count;

    GeneratorTransport in_png_trans(source_png.from_offset(8));

    RDPDrawable d(20, 10);
    gdi::GraphicApi * gdi = &d;
    set_rows_from_image_chunk(in_png_trans, WrmChunkType(chunk_type), chunk_size, d.width(), {&gdi, 1});

    BufTransport png_trans;
    ::dump_png24(png_trans, d, true);
    RED_TEST(png_trans.size() == 107);
}

RED_AUTO_TEST_CASE(TestSwitchTitleExtractor)
{
    force_paris_timezone();
    WorkingDirectory hash_wd("hash");
    WorkingDirectory record_wd("record");

    TestCaptureContext ctx_cap("title_extractor", CaptureFlags{}, 800, 600,
        record_wd, hash_wd, KbdLogParams());
    ctx_cap.capture_meta = true;
    ctx_cap.capture_ocr = true;

    ctx_cap.run([&](Capture& capture, Rect scr){
        MonotonicTimePoint now{1000s};
        now += 100s;

        auto draw_img = [&](char const* filename){
            Bitmap img;
            RED_CHECK((img = bitmap_from_file(filename, NamedBGRColor::BLACK)).is_valid());
            capture.draw(
                RDPMemBlt(0, Rect(0, 0, img.cx(), img.cy()), 0xCC, 0, 0, 0),
                scr, img);
        };

        // gestionnaire de serveur
        draw_img(FIXTURES_PATH "/m-21288-2.png");
        capture.periodic_snapshot(now, 0, 0);

        now += 100s;
        capture.session_update(now, LogId::PROBE_STATUS, {
            KVLog("status"_av, "Unknown"_av),
        });
        capture.periodic_snapshot(now, 0, 0);

        now += 100s;
        capture.session_update(now, LogId::PROBE_STATUS, {
            KVLog("status"_av, "Ready"_av),
        });
        capture.session_update(now, LogId::FOREGROUND_WINDOW_CHANGED, {
            KVLog("windows"_av, "a"_av),
            KVLog("class"_av, "t"_av),
            KVLog("command_line"_av, "u"_av),
        });

        // qwhybcaliueLkaASsFkkUibnkzkwwkswq.txt - Bloc-notes
        draw_img(FIXTURES_PATH "/win2008capture10.png");
        capture.periodic_snapshot(now, 0, 0);
        now += 100s;
        capture.session_update(now, LogId::FOREGROUND_WINDOW_CHANGED, {
            KVLog("windows"_av, "b"_av),
            KVLog("class"_av, "x"_av),
            KVLog("command_line"_av, "y"_av),
        });
        capture.periodic_snapshot(now, 0, 0);

        now += 100s;
        // disable sespro
        capture.session_update(now, LogId::PROBE_STATUS, {
            KVLog("status"_av, "Unknown"_av),
        });
        // not TITLE_BAR extractor with OCR
        capture.session_update(now, LogId::FOREGROUND_WINDOW_CHANGED, {
            KVLog("windows"_av, "c"_av),
            KVLog("class"_av, "t"_av),
            KVLog("command_line"_av, "v"_av),
        });
        capture.periodic_snapshot(now, 0, 0);

        now += 100s;
        // Gestionnaire de licences TS
        draw_img(FIXTURES_PATH "/win2008capture.png");
        capture.periodic_snapshot(now, 0, 0);
    });

    auto meta_content =
        R"(1970-01-01 01:18:20 + type="TITLE_BAR" data="Gestionnaire de serveur")" "\n"
        R"(1970-01-01 01:21:40 - type="FOREGROUND_WINDOW_CHANGED" windows="a" class="t" command_line="u")" "\n"
        R"(1970-01-01 01:21:40 + type="TITLE_BAR" data="a")" "\n"
        R"(1970-01-01 01:23:20 - type="FOREGROUND_WINDOW_CHANGED" windows="b" class="x" command_line="y")" "\n"
        R"(1970-01-01 01:23:20 + type="TITLE_BAR" data="b")" "\n"
        R"(1970-01-01 01:25:00 - type="FOREGROUND_WINDOW_CHANGED" windows="c" class="t" command_line="v")" "\n"
        R"(1970-01-01 01:25:00 + type="TITLE_BAR" data="qwhybcaliueLkaASsFkkUibnkzkwwkswq.txt - Bloc-notes")" "\n"
        R"(1970-01-01 01:26:40 + type="TITLE_BAR" data="Gestionnaire de licences TS")" "\n"
        ""_av;

    RED_CHECK_FILE_CONTENTS(record_wd.add_file("title_extractor.meta"), meta_content);

    RED_CHECK_WORKSPACE(hash_wd);
    RED_CHECK_WORKSPACE(record_wd);
}
