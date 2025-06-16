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

   Unit test to capture interface to video recording to flv or mp4
*/

#ifndef REDEMPTION_NO_FFMPEG
#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"
#include "test_only/test_framework/check_img.hpp"
#include "test_only/test_framework/file.hpp"
#include "test_only/force_paris_timezone.hpp"

#include "capture/video_capture.hpp"

#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/caches/pointercache.hpp"
#include "capture/full_video_params.hpp"
#include "capture/sequenced_video_params.hpp"
#include "capture/capture_params.hpp"
#include "core/RDP/RDPDrawable.hpp"
#include "utils/fileutils.hpp"
#include "capture/lazy_drawable_pointer.hpp"

#include <chrono>

#define IMG_TEST_PATH FIXTURES_PATH "/img_ref/capture/video_capture/"

using namespace std::chrono_literals;

namespace
{
    void simple_movie(
        MonotonicTimePoint now, unsigned duration, RDPDrawable & drawable,
        LazyDrawablePointer & lazy_drawable_pointer,
        gdi::CaptureApi & capture, gdi::GraphicApi & video_drawable, bool mouse
    ) {
        force_paris_timezone();

        Rect screen(0, 0, drawable.width(), drawable.height());
        auto const color_cxt = gdi::ColorCtx::depth24();
        drawable.draw(RDPOpaqueRect(screen, encode_color24()(NamedBGRColor::BLUE)), screen, color_cxt);
        video_drawable.draw(RDPOpaqueRect(screen, encode_color24()(NamedBGRColor::BLUE)), screen, color_cxt);

        lazy_drawable_pointer.set_position(drawable.width() / 2, drawable.height() / 2);

        Rect r(10, 10, 50, 50);
        int vx = 5;
        int vy = 4;
        for (size_t x = 0; x < duration; x++) {
            drawable.draw(RDPOpaqueRect(r, encode_color24()(NamedBGRColor::BLUE)), screen, color_cxt);
            r.y += vy;
            r.x += vx;
            drawable.draw(RDPOpaqueRect(r, encode_color24()(NamedBGRColor::WABGREEN)), screen, color_cxt);
            video_drawable.draw(RDPOpaqueRect(r, encode_color24()(NamedBGRColor::WABGREEN)), screen, color_cxt);
            now += 40000us;
            //printf("now sec=%u usec=%u\n", (unsigned)now.tv_sec, (unsigned)now.tv_usec);
            uint16_t cursor_x = mouse ? uint16_t(r.x + 10) : 0;
            uint16_t cursor_y = mouse ? uint16_t(r.y + 10) : 0;
            lazy_drawable_pointer.set_position(cursor_x, cursor_y);
            capture.periodic_snapshot(now, cursor_x, cursor_y);
            capture.periodic_snapshot(now, cursor_x, cursor_y);
            if ((r.x + r.cx >= drawable.width())  || (r.x < 0)) { vx = -vx; }
            if ((r.y + r.cy >= drawable.height()) || (r.y < 0)) { vy = -vy; }
        }
        // last frame (video.encoding_video_frame())
        now += 40000us;
        uint16_t cursor_x = mouse ? uint16_t(r.x + 10) : 0;
        uint16_t cursor_y = mouse ? uint16_t(r.y + 10) : 0;
        capture.periodic_snapshot(now, cursor_x, cursor_y);
    }

    struct notified_on_video_change : public NotifyNextVideo
    {
        void notify_next_video(MonotonicTimePoint /*now*/, Reason /*reason*/) override
        {
        }
    } next_video_notifier;

    struct Codec
    {
        char const* name;
        char const* options;
    };
    constexpr Codec mp4{"mp4", "profile=baseline preset=ultrafast b=100000"};

    enum class Cropped : bool;
    enum class Mouse : bool;

    Rect to_rect(RDPDrawable& drawable, Cropped cropped)
    {
        return bool(cropped)
            ? Rect(drawable.width() / 4, drawable.height() / 4,
                   drawable.width() / 2 + 1, drawable.height() / 2 + 1)
            : Rect(0, 0, drawable.width(), drawable.height());
    }

    void simple_sequenced_video(
        char const* dirname, Codec const& codec, std::chrono::seconds video_interval,
        unsigned loop_duration, Mouse mouse, Cropped cropped)
    {
        MonotonicTimePoint monotonic_time{12s + 653432us};
        RealTimePoint real_time{1353055788s + monotonic_time.time_since_epoch()};
        RDPDrawable drawable(800, 600);
        PointerCache ptr_cache(15);
        LazyDrawablePointer lazy_drawable_pointer(ptr_cache.source_pointers_view());
        VideoParams video_params{
            25, codec.name, codec.options, false, 0};
        SequencedVideoParams sequenced_video_params { video_interval };
        CaptureParams capture_params{
            monotonic_time, real_time, "video", nullptr, dirname,
            FilePermissions::user_permissions(BitPermissions::read),
            nullptr, SmartVideoCropping::disable, 0};
        SequencedVideoCaptureImpl video_capture(
            capture_params, 0 /* png_width */, 0 /* png_height */,
            drawable.impl(), lazy_drawable_pointer, to_rect(drawable, cropped),
            video_params, sequenced_video_params, next_video_notifier);
        simple_movie(
            monotonic_time, loop_duration, drawable, lazy_drawable_pointer,
            video_capture, video_capture.graphics_api(), bool(mouse));
    }

    void simple_full_video(
        char const* dirname, Codec const& codec,
        unsigned loop_duration, Mouse mouse, Cropped cropped)
    {
        MonotonicTimePoint monotonic_time{12s + 653432us};
        RealTimePoint real_time{1353055788s + monotonic_time.time_since_epoch()};
        RDPDrawable drawable(800, 600);
        PointerCache ptr_cache(15);
        LazyDrawablePointer lazy_drawable_pointer(ptr_cache.source_pointers_view());
        VideoParams video_params{
            25, codec.name, codec.options, false, 0};
        CaptureParams capture_params{
            monotonic_time, real_time, "video", nullptr, dirname,
            FilePermissions::user_and_group_permissions(BitPermissions::read),
            nullptr, SmartVideoCropping::disable, 0};
        FullVideoCaptureImpl video_capture(
            capture_params, drawable.impl(), lazy_drawable_pointer, to_rect(drawable, cropped),
            video_params, FullVideoParams{});
        simple_movie(
            monotonic_time, loop_duration, drawable, lazy_drawable_pointer,
            video_capture, video_capture.graphics_api(), bool(mouse));
    }
} // namespace

RED_AUTO_TEST_CASE_WD(TestSequencedVideoCaptureMP4, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 2s, 250, Mouse(true), Cropped(false));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "2bis.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "2s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "4s.png");
    RED_CHECK_IMG(wd.add_file("video-000003.png"), IMG_TEST_PATH "6s.png");
    RED_CHECK_IMG(wd.add_file("video-000004.png"), IMG_TEST_PATH "8s.png");
    RED_CHECK_IMG(wd.add_file("video-000005.png"), IMG_TEST_PATH "10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 23021 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 23000 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 23267 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000003.mp4"), 23767 +- 2500_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000004.mp4"), 23044 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000005.mp4"), 5315 +- 2000_v);
}

RED_AUTO_TEST_CASE_WD(SequencedVideoCaptureX264, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 1s, 250, Mouse(true), Cropped(false));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "1s.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "1s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "2s.png");
    RED_CHECK_IMG(wd.add_file("video-000003.png"), IMG_TEST_PATH "3s.png");
    RED_CHECK_IMG(wd.add_file("video-000004.png"), IMG_TEST_PATH "4s.png");
    RED_CHECK_IMG(wd.add_file("video-000005.png"), IMG_TEST_PATH "5s.png");
    RED_CHECK_IMG(wd.add_file("video-000006.png"), IMG_TEST_PATH "6s.png");
    RED_CHECK_IMG(wd.add_file("video-000007.png"), IMG_TEST_PATH "7s.png");
    RED_CHECK_IMG(wd.add_file("video-000008.png"), IMG_TEST_PATH "8s.png");
    RED_CHECK_IMG(wd.add_file("video-000009.png"), IMG_TEST_PATH "9s.png");
    RED_CHECK_IMG(wd.add_file("video-000010.png"), IMG_TEST_PATH "10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 13584 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 15175 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 15299 +- 2400_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000003.mp4"), 13576 +- 2700_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000004.mp4"), 13587 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000005.mp4"), 14264 +- 2600_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000006.mp4"), 13949 +- 3100_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000007.mp4"), 13385 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000008.mp4"), 13622 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000009.mp4"), 13693 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000010.mp4"), 6080 +- 200_v);
}

RED_AUTO_TEST_CASE_WD(TestSequencedVideoCaptureMP4_3, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 5s, 250, Mouse(true), Cropped(false));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "2bis.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "5s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 53021 +- 3100_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 54338 +- 3700_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 6080 +- 200_v);
}

RED_AUTO_TEST_CASE_WD(TestFullVideoCaptureX264, wd)
{
    simple_full_video(wd.dirname(), mp4, 250, Mouse(true), Cropped(false));

    RED_TEST_FILE_SIZE(wd.add_file("video.mp4"), 106930 +- 10000_v);
}

RED_AUTO_TEST_CASE_WD(TestFullVideoCaptureX264_2, wd)
{
    simple_full_video(wd.dirname(), mp4, 250, Mouse(false), Cropped(false));

    RED_TEST_FILE_SIZE(wd.add_file("video.mp4"), 92693 +- 10000_v);
}


RED_AUTO_TEST_CASE_WD(TestSequencedVideoCaptureCroppedMP4, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 2s, 250, Mouse(true), Cropped(true));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "cropped_2bis.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "cropped_2s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "cropped_4s.png");
    RED_CHECK_IMG(wd.add_file("video-000003.png"), IMG_TEST_PATH "cropped_6s.png");
    RED_CHECK_IMG(wd.add_file("video-000004.png"), IMG_TEST_PATH "cropped_8s.png");
    RED_CHECK_IMG(wd.add_file("video-000005.png"), IMG_TEST_PATH "cropped_10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 20021 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 27338 +- 2500_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 15267 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000003.mp4"), 19767 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000004.mp4"), 28044 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000005.mp4"), 5315 +- 2000_v);
}

RED_AUTO_TEST_CASE_WD(SequencedVideoCaptureCroppedX264, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 1s, 250, Mouse(true), Cropped(true));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "cropped_1s.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "cropped_1s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "cropped_2s.png");
    RED_CHECK_IMG(wd.add_file("video-000003.png"), IMG_TEST_PATH "cropped_3s.png");
    RED_CHECK_IMG(wd.add_file("video-000004.png"), IMG_TEST_PATH "cropped_4s.png");
    RED_CHECK_IMG(wd.add_file("video-000005.png"), IMG_TEST_PATH "cropped_5s.png");
    RED_CHECK_IMG(wd.add_file("video-000006.png"), IMG_TEST_PATH "cropped_6s.png");
    RED_CHECK_IMG(wd.add_file("video-000007.png"), IMG_TEST_PATH "cropped_7s.png");
    RED_CHECK_IMG(wd.add_file("video-000008.png"), IMG_TEST_PATH "cropped_8s.png");
    RED_CHECK_IMG(wd.add_file("video-000009.png"), IMG_TEST_PATH "cropped_9s.png");
    RED_CHECK_IMG(wd.add_file("video-000010.png"), IMG_TEST_PATH "cropped_10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 8584 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 15175 +- 2100_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 15299 +- 3000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000003.mp4"), 15576 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000004.mp4"), 11587 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000005.mp4"), 8264 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000006.mp4"), 8949 +- 3100_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000007.mp4"), 15385 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000008.mp4"), 17622 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000009.mp4"), 17693 +- 2000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000010.mp4"), 4980 +- 300_v);
}

RED_AUTO_TEST_CASE_WD(TestSequencedVideoCaptureCroppedMP4_3, wd)
{
    simple_sequenced_video(wd.dirname(), mp4, 5s, 250, Mouse(true), Cropped(true));

    RED_CHECK_IMG(wd.add_file("video-000000.png"), IMG_TEST_PATH "cropped_2bis.png");
    RED_CHECK_IMG(wd.add_file("video-000001.png"), IMG_TEST_PATH "cropped_5s.png");
    RED_CHECK_IMG(wd.add_file("video-000002.png"), IMG_TEST_PATH "cropped_10s.png");
    RED_TEST_FILE_SIZE(wd.add_file("video-000000.mp4"), 49021 +- 6000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000001.mp4"), 46338 +- 6000_v);
    RED_TEST_FILE_SIZE(wd.add_file("video-000002.mp4"), 4980 +- 300_v);
}

RED_AUTO_TEST_CASE_WD(TestFullVideoCaptureCroppedX264, wd)
{
    simple_full_video(wd.dirname(), mp4, 250, Mouse(true), Cropped(true));

    RED_TEST_FILE_SIZE(wd.add_file("video.mp4"), 86930 +- 10000_v);
}

RED_AUTO_TEST_CASE_WD(TestFullVideoCaptureCroppedX264_2, wd)
{
    simple_full_video(wd.dirname(), mp4, 250, Mouse(false), Cropped(true));

    RED_TEST_FILE_SIZE(wd.add_file("video.mp4"), 65693 +- 10000_v);
}
#else
int main() {}
#endif
