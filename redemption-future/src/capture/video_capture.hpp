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
   Copyright (C) Wallix 2017
   Author(s): Christophe Grosjean, Jonatan Poelen
*/

#pragma once

#include "acl/auth_api.hpp"
#include "capture/video_params.hpp"
#include "capture/video_recorder.hpp"
#include "capture/notify_next_video.hpp"
#include "capture/lazy_drawable_pointer.hpp"
#include "gdi/capture_api.hpp"
#include "gdi/rect_tracker.hpp"
#include "utils/sugar/noncopyable.hpp"
#include "utils/timestamp_tracer.hpp"
#include "utils/monotonic_time_to_real_time.hpp"
#include "utils/scaled_image24.hpp"
#include "utils/bitset_stream.hpp"
#include "utils/drawable_pointer.hpp"

#include <chrono>
#include <optional>

class CaptureParams;
class FullVideoParams;
class SequencedVideoParams;
class Drawable;

struct VideoCaptureCtx : noncopyable
{
    enum class ImageByInterval : bool
    {
        ZeroOrOneWithTimestamp,
        ZeroOrOneWithoutTimestamp,
    };

    VideoCaptureCtx(
        MonotonicTimePoint monotonic_now,
        RealTimePoint real_now,
        ImageByInterval image_by_interval,
        unsigned frame_rate,
        Drawable & drawable,
        LazyDrawablePointer & lazy_drawable_pointer,
        Rect crop_rect,
        array_view<BitsetInStream::underlying_type> updatable_frame_marker_end_bitset_view
    );

    gdi::GraphicApi& graphics_api() noexcept
    {
        return this->rect_tracker;
    }

    void frame_marker_event(
        video_recorder & recorder, MonotonicTimePoint now,
        uint16_t cursor_x, uint16_t cursor_y);

    gdi::CaptureApi::WaitingTimeBeforeNextSnapshot snapshot(
        video_recorder& recorder, MonotonicTimePoint now,
        uint16_t cursor_x, uint16_t cursor_y);

    void encoding_end_frame(video_recorder & recorder);

    void next_video(video_recorder & recorder);

    void synchronize_times(MonotonicTimePoint monotonic_time, RealTimePoint real_time);

    void set_cropping(Rect cropping) noexcept;

    bool logical_frame_ended() const noexcept;

    WritableImageView acquire_image_for_dump(
        DrawablePointer::BufferSaver& buffer_saver,
        const tm& now);

    void release_image_for_dump(
        WritableImageView image,
        DrawablePointer::BufferSaver const& buffer_saver);

    tm get_tm() const;

    void update_fullscreen();

private:
    void preparing_video_frame(video_recorder & recorder);

    [[nodiscard]]
    WritableImageView prepare_image_frame() noexcept;

    struct VideoCropper
    {
        VideoCropper(Drawable & drawable, Rect crop_rect);

        void set_cropping(Rect cropping) noexcept;

        bool is_cropped() const noexcept
        {
            return !this->is_fullscreen;
        }

        void prepare_image_frame(Drawable & drawable) noexcept;

        WritableImageView get_image(Drawable& drawable) noexcept;

    private:
        Rect crop_rect;
        Dimension original_dimension;
        bool is_fullscreen;
        std::unique_ptr<uint8_t[]> out_bmpdata;
    };

    Drawable & drawable;
    LazyDrawablePointer & lazy_drawable_pointer;
    MonotonicTimePoint monotonic_last_time_capture;
    MonotonicTimeToRealTime monotonic_to_real;
    MonotonicTimePoint::duration frame_interval;

    MonotonicTimePoint next_trace_time;
    int64_t frame_index = 0;
    const ImageByInterval image_by_interval;
    const bool has_timestamp;
    bool has_frame_marker = false;
    uint16_t cursor_x = 0;
    uint16_t cursor_y = 0;

    VideoCropper video_cropper;

    RectTracker rect_tracker;
    BitsetInStream updatable_frame_marker_end_bitset_stream;
    BitsetInStream::underlying_type const* updatable_frame_marker_end_bitset_end;

public:
    TimestampTracer timestamp_tracer;
};

class FullVideoCaptureImpl final : public gdi::CaptureApi
{
public:
    FullVideoCaptureImpl(
        CaptureParams const & capture_params,
        Drawable & drawable,
        LazyDrawablePointer & lazy_drawable_pointer,
        Rect crop_rect,
        VideoParams const & video_params,
        FullVideoParams const & full_video_params
    );

    ~FullVideoCaptureImpl();

    gdi::GraphicApi& graphics_api() noexcept
    {
        return this->video_cap_ctx.graphics_api();
    }

    void set_cropping(Rect cropping)
    {
        this->video_cap_ctx.set_cropping(cropping);
    }

    void frame_marker_event(
        MonotonicTimePoint now, uint16_t cursor_x, uint16_t cursor_y
    ) override;

    WaitingTimeBeforeNextSnapshot periodic_snapshot(
        MonotonicTimePoint now, uint16_t cursor_x, uint16_t cursor_y
    ) override;

    void synchronize_times(MonotonicTimePoint monotonic_time, RealTimePoint real_time);

    void update_fullscreen()
    {
        this->video_cap_ctx.update_fullscreen();
    }

private:
    VideoCaptureCtx video_cap_ctx;
    video_recorder recorder;
};


class SequencedVideoCaptureImpl final : public gdi::CaptureApi
{
public:
    SequencedVideoCaptureImpl(
        CaptureParams const & capture_params,
        unsigned png_width, unsigned png_height,
        Drawable & drawable,
        LazyDrawablePointer & lazy_drawable_pointer,
        Rect crop_rect,
        VideoParams const& video_params,
        SequencedVideoParams const& sequenced_video_params,
        NotifyNextVideo & next_video_notifier);

    ~SequencedVideoCaptureImpl();

    gdi::GraphicApi& graphics_api() noexcept
    {
        return this->video_cap_ctx.graphics_api();
    }

    void set_cropping(Rect cropping)
    {
        this->video_cap_ctx.set_cropping(cropping);
    }

    void frame_marker_event(
        MonotonicTimePoint now, uint16_t cursor_x, uint16_t cursor_y
    ) override;

    WaitingTimeBeforeNextSnapshot periodic_snapshot(
        MonotonicTimePoint now,
        uint16_t cursor_x, uint16_t cursor_y
    ) override;

    void next_video(MonotonicTimePoint now);

    void synchronize_times(MonotonicTimePoint monotonic_time, RealTimePoint real_time);

    void update_fullscreen()
    {
        this->video_cap_ctx.update_fullscreen();
    }

private:
    void ic_flush(const tm& now);

    void next_video_impl(MonotonicTimePoint now, NotifyNextVideo::Reason reason);

    // first next_video is ignored
    WaitingTimeBeforeNextSnapshot first_periodic_snapshot(MonotonicTimePoint now);
    WaitingTimeBeforeNextSnapshot video_sequencer_periodic_snapshot(MonotonicTimePoint now);

    void init_recorder();

    struct FilenameGenerator
    {
        FilenameGenerator(
            std::string_view prefix,
            std::string_view filename,
            std::string_view extension
        );

        void next();
        char const* current_filename() const;

    private:
        std::string filename;
        int const num_pos;
        int num = 0;
    };

    bool ic_has_first_img = false;

    const MonotonicTimePoint monotonic_start_capture;
    MonotonicTimeToRealTime monotonic_to_real;

    VideoCaptureCtx video_cap_ctx;
    FilenameGenerator vc_filename_generator;
    std::optional<video_recorder> recorder;
    FilenameGenerator ic_filename_generator;

    ScaledPng24 ic_scaled_png;

    MonotonicTimePoint start_break;
    const std::chrono::microseconds break_interval;

    NotifyNextVideo & next_video_notifier;

    struct RecorderParams
    {
        AclReportApi * acl_report;
        std::string codec_name;
        std::string codec_options;
        int frame_rate;
        int verbosity;
        FilePermissions file_permissions;
    };

    const RecorderParams recorder_params;
};
