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
   Copyright (C) Wallix 2010-2019
   Author(s): Christophe Grosjean, Jonathan Poelen
*/

// TODO: this should move to src/system as results are highly dependent on compilation system
// maybe some external utility could detect the target/variant of library and could avoid
// non determinist sizes in generated movies.

#include "capture/video_recorder.hpp"

#ifndef REDEMPTION_NO_FFMPEG

extern "C" {
    // On Debian lenny and on higher debian/ubuntu distribution, ffmpeg includes
    // aren't localized on the same path (usr/include/ffmpeg for lenny,
    // /usr/include/libXXX for ubuntu/debian testing/unstable)
    #ifndef UINT64_C
    #define UINT64_C uint64_t
    #endif

    #include <libavutil/avutil.h>
    #include <libavutil/dict.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include "core/error.hpp"
#include "cxx/diagnostic.hpp"
#include "utils/image_view.hpp"
#include "utils/sugar/scope_exit.hpp"
#include "utils/sugar/unique_fd.hpp"
#include "utils/strutils.hpp"
#include "utils/log.hpp"
#include "acl/auth_api.hpp"

#include <algorithm>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace
{
    constexpr int original_timestamp_height = 12;

    struct default_av_free
    {
        void operator()(void * ptr) noexcept
        {
            av_free(ptr);
        }
    };

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 65, 0)
    // not sure which version of libavcodec
    void avio_context_free(AVIOContext** ptr)
    {
        av_free(*ptr);
        *ptr = nullptr;
    }
#endif

    void video_transport_acl_report(AclReportApi * acl_report, int errnum)
    {
        if (errnum == ENOSPC) {
            if (acl_report){
                acl_report->acl_report(AclReport::file_system_full());
            }
            else {
                LOG(LOG_ERR, "FILESYSTEM_FULL:100|unknown");
            }
        }
    }

    struct VideoRecorderOutputFile
    {
        explicit VideoRecorderOutputFile(
            char const* filename,
            FilePermissions file_permissions,
            AclReportApi * acl_report)
        : fd{[acl_report, filename, mode = file_permissions.permissions_as_uint()]{
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
            if (fd == -1) {
                int const errnum = errno;
                LOG( LOG_ERR, "can't open file %s: %s [%d]"
                   , filename, strerror(errnum), errnum);
                video_transport_acl_report(acl_report, errnum);
                throw Error(ERR_TRANSPORT_OPEN_FAILED, errnum);
            }

            return unique_fd{fd};
        }()}
        , acl_report(acl_report)
        {
        }

        int write(uint8_t const* buf, int buf_size)
        {
            assert(buf_size >= 0);

            if (buf_size + this->buffer_len < buffer_capacity) {
                memcpy(this->buffer.get() + this->buffer_len, buf, static_cast<size_t>(buf_size));
                this->buffer_len += buf_size;
                return buf_size;
            }

            iovec iov[]{
                {this->buffer.get(), static_cast<size_t>(this->buffer_len)},
                {const_cast<uint8_t*>(buf), static_cast<size_t>(buf_size)}
            };
            ssize_t len = writev(this->fd.fd(), iov, 2);
            ssize_t total = this->buffer_len + buf_size;
            this->buffer_len = 0;

            if (REDEMPTION_UNLIKELY(len != total)) {
                return log_error("send");
            }

            this->buffer_len = 0;

            return buf_size;
        }

        int64_t seek(int64_t offset, int whence)
        {
            // This function is like the fseek() C stdio function.
            // "whence" can be either one of the standard C values
            // (SEEK_SET, SEEK_CUR, SEEK_END) or one more value: AVSEEK_SIZE.

            // When "whence" has this value, your seek function must
            // not seek but return the size of your file handle in bytes.
            // This is also optional and if you don't implement it you must return <0.

            // Otherwise you must return the current position of your stream
            //  in bytes (that is, after the seeking is performed).
            // If the seek has failed you must return <0.

            if (REDEMPTION_UNLIKELY(whence == AVSEEK_SIZE)) {
                LOG(LOG_ERR, "Video seek failure");
                return -1;
            }

            if (REDEMPTION_UNLIKELY(this->send_remaining_buffer() < 0)) {
                return -1;
            }

            auto res = lseek64(this->fd.fd(), offset, whence);
            if (REDEMPTION_UNLIKELY(res == static_cast<off_t>(-1))) {
                return log_error("seek");
            }

            return offset;
        }

        void close()
        {
            this->send_remaining_buffer();
            this->fd.close();
        }

    private:
        int send_remaining_buffer()
        {
            if (this->buffer_len) {
                ssize_t len = ::write(this->fd.fd(), this->buffer.get(), size_t(this->buffer_len));
                ssize_t total = this->buffer_len;
                this->buffer_len = 0;

                if (REDEMPTION_UNLIKELY(len != total)) {
                    return log_error("send");
                }
            }

            return 0;
        }

        int log_error(char const* funcname)
        {
            int errnum = errno;
            LOG(LOG_ERR, "VideoRecorder::%s: %s [%d]", funcname, strerror(errnum), errnum);
            video_transport_acl_report(this->acl_report, errnum);
            return -1;
        }

        static const int buffer_capacity = 1024*256;

        unique_fd fd;
        int buffer_len = 0;
        std::unique_ptr<uint8_t[]> buffer{new uint8_t[buffer_capacity]} /*NOLINT*/;
        AclReportApi * acl_report;
    };
} // anonymous namespace

struct video_recorder::D
{
    VideoRecorderOutputFile out_file;

    AVStream* video_st = nullptr;

    AVFrame* dst_frame = nullptr;
    uint8_t *src_frame_data[AV_NUM_DATA_POINTERS];
    int src_frame_linesize[AV_NUM_DATA_POINTERS];

    AVCodecContext* codec_ctx = nullptr;
    AVFormatContext* oc = nullptr;
    SwsContext* frame_convert_ctx = nullptr;
    SwsContext* timestamp_convert_ctx = nullptr;

    AVPacket* pkt = nullptr;
    int original_height;
    int previous_height;
    int timestamp_height;

    std::unique_ptr<uint8_t, default_av_free> dst_frame_buffer;

    /* custom IO */
    std::unique_ptr<uint8_t, default_av_free> custom_io_buffer;
    AVIOContext* custom_io_context = nullptr;

    D(char const* filename, FilePermissions file_permissions, AclReportApi * acl_report)
    : out_file(filename, file_permissions, acl_report)
    , dst_frame(av_frame_alloc())
    , pkt(av_packet_alloc())
    {}

    ~D()
    {
        this->out_file.close();
        avio_context_free(&this->custom_io_context);
        sws_freeContext(this->frame_convert_ctx);
        sws_freeContext(this->timestamp_convert_ctx);
        avformat_free_context(this->oc);
        avcodec_free_context(&this->codec_ctx);
        av_frame_free(&this->dst_frame);
        av_packet_free(&this->pkt);
    }
};

static void throw_if(bool test, char const* msg, char const* extra_msg = "")
{
    if (test) {
        LOG(LOG_ERR, "video recorder: %s%s", msg, extra_msg);
        throw Error(ERR_VIDEO_RECORDER);
    }
}

static void check_errnum(int errnum, char const* msg)
{
    if (REDEMPTION_UNLIKELY(errnum < 0)) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE]{};
        LOG(LOG_ERR, "video recorder: %s: %s",
            msg, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, errnum));
        throw Error(ERR_VIDEO_RECORDER);
    }
}

// https://libav.org/documentation/doxygen/master/output_8c-example.html
// https://libav.org/documentation/doxygen/master/encode_video_8c-example.html

video_recorder::video_recorder(
    char const* filename, FilePermissions file_permissions, AclReportApi * acl_report,
    ImageView const& image_view, int frame_rate,
    const char * codec_name, char const* codec_options, int log_level
)
: d(std::make_unique<D>(filename, file_permissions, acl_report))
{
    const int width = image_view.width();
    const int height = image_view.height();

    d->original_height = height;
    d->previous_height = height;

    #if LIBAVCODEC_VERSION_MAJOR <= 57
    /* initialize libavcodec, and register all codecs and formats */
    av_register_all();
    #endif

    av_log_set_level(log_level);


    this->d->oc = avformat_alloc_context();
    throw_if(!this->d->oc, "Failed allocating output media context");

    /* auto detect the output format from the name. default is mpeg. */
    // AVOutputFormat* <= ffmpeg4.4 ; AVOutputFormat const* > ffmpeg4.4
    auto* fmt = av_guess_format(codec_name, nullptr, nullptr);
    throw_if(!fmt || fmt->video_codec == AV_CODEC_ID_NONE, "Could not find codec ", codec_name);

    const auto codec_id = fmt->video_codec;
    const AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;
    const AVPixelFormat src_pix_fmt = AV_PIX_FMT_BGR24;

    this->d->oc->oformat = fmt;

    // add the video streams using the default format codecs and initialize the codecs
    this->d->video_st = avformat_new_stream(this->d->oc, nullptr);
    throw_if(!this->d->video_st, "Could not find suitable output format");

    this->d->video_st->time_base = AVRational{1, frame_rate};

    // AVCodec* <= ffmpeg4.4 ; AVCodec const* > ffmpeg4.4
    auto* codec = avcodec_find_encoder(codec_id);
    throw_if(!codec, "Codec not found");

    this->d->codec_ctx = avcodec_alloc_context3(codec);

    this->d->codec_ctx->codec_id = codec_id;
    this->d->codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    // this->d->codec_ctx->bit_rate = bitrate;
    // this->d->codec_ctx->bit_rate_tolerance = bitrate;
    // resolution must be a multiple of 2
    this->d->codec_ctx->width = width & ~1;
    this->d->codec_ctx->height = height & ~1;

    // time base: this is the fundamental unit of time (in seconds)
    // in terms of which frame timestamps are represented.
    // for fixed-fps content, timebase should be 1/framerate
    // and timestamp increments should be identically 1
    this->d->codec_ctx->time_base = AVRational{1, frame_rate};

    // impact: keyframe, filesize and time of generating
    // high value = ++time, --size
    // keyframe managed by this->d->pkt.flags |= AV_PKT_FLAG_KEY and av_interleaved_write_frame
    this->d->codec_ctx->gop_size = std::max(2, frame_rate);

    this->d->codec_ctx->pix_fmt = dst_pix_fmt;

    REDEMPTION_DIAGNOSTIC_PUSH()
    REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wswitch")
    switch (codec_id){
        case AV_CODEC_ID_H264:
            //codec_ctx->coder_type = FF_CODER_TYPE_AC;
            //codec_ctx->flags2 = CODEC_FLAG2_WPRED | CODEC_FLAG2_MIXED_REFS |
            //                                CODEC_FLAG2_8X8DCT | CODEC_FLAG2_FASTPSKIP;

            //codec_ctx->partitions = X264_PART_I8X8 | X264_PART_P8X8 | X264_PART_I4X4;
            this->d->codec_ctx->me_range = 16;
            //codec_ctx->refs = 1;
            //codec_ctx->flags = CODEC_FLAG_4MV | CODEC_FLAG_LOOP_FILTER;
            // this->d->codec_ctx->flags |= AVFMT_NOTIMESTAMPS;
            this->d->codec_ctx->qcompress = 0.0;
            this->d->codec_ctx->max_qdiff = 4;
        break;
        case AV_CODEC_ID_MPEG1VIDEO:
            // Needed to avoid using macroblocks in which some coeffs overflow.
            // This does not happen with normal video, it just happens here as
            // the motion of the chroma plane does not match the luma plane.
            this->d->codec_ctx->mb_decision = 2;
        break;
    }
    REDEMPTION_DIAGNOSTIC_POP()

    // some formats want stream headers to be separate
    this->d->codec_ctx->flags |= (fmt->flags & AVFMT_GLOBALHEADER) ? AV_CODEC_FLAG_GLOBAL_HEADER : 0;

    // dump_format can be handy for debugging
    // it dump information about file to stderr
    // dump_format(this->d->oc, 0, file, 1);

    // ************** open_video ****************
    // now we can open the audio and video codecs
    // and allocate the necessary encode buffers
    // find the video encoder

    {
        AVDictionary *av_dict = nullptr;
        SCOPE_EXIT(av_dict_free(&av_dict));
        check_errnum(av_dict_parse_string(&av_dict, codec_options, "=", " ", 0),
            "av_dict_parse_string error");

        check_errnum(avcodec_open2(this->d->codec_ctx, codec, &av_dict),
            "Failed to open codec, possible bad codec option");
    }

    check_errnum(avcodec_parameters_from_context(this->d->video_st->codecpar, this->d->codec_ctx),
        "Failed to copy codec parameters");

    throw_if(
        av_new_packet(this->d->pkt, av_image_get_buffer_size(src_pix_fmt, width, height, 1)),
        "Failed to allocate pkt buf");

    // init picture frame
    if (int const size = av_image_get_buffer_size(dst_pix_fmt, width, height, 1)) {
        this->d->dst_frame_buffer.reset(static_cast<uint8_t*>(av_malloc(size)));
        std::fill_n(this->d->dst_frame_buffer.get(), size, uint8_t());
    }

    throw_if(!this->d->dst_frame_buffer, "Failed to allocate picture buf");

    av_image_fill_arrays(
        this->d->dst_frame->data, this->d->dst_frame->linesize,
        this->d->dst_frame_buffer.get(), dst_pix_fmt, width, height, 1
    );

    this->d->dst_frame->width = width;
    this->d->dst_frame->height = height;
    this->d->dst_frame->format = dst_pix_fmt;

    const std::size_t io_buffer_size = 128 * 1024;

    this->d->custom_io_buffer.reset(static_cast<unsigned char *>(av_malloc(io_buffer_size)));
    throw_if(!this->d->custom_io_buffer, "Failed to allocate io");

    this->d->custom_io_context = avio_alloc_context(
        this->d->custom_io_buffer.get(), // buffer
        io_buffer_size,               // buffer size
        1,                            // writable
        &this->d->out_file,           // user-specific data
        nullptr,                      // function for refilling the buffer, may be nullptr.
        [](void * data, auto /* uint8_t const or not... */ * buf, int buf_size) {
            return static_cast<VideoRecorderOutputFile*>(data)->write(buf, buf_size);
        },
        [](void * data, int64_t offset, int whence) {
            return static_cast<VideoRecorderOutputFile*>(data)->seek(offset, whence);
        }
    );
    throw_if(!this->d->custom_io_context, "Failed to allocate AVIOContext");

    this->d->oc->pb = this->d->custom_io_context;

    check_errnum(avformat_write_header(this->d->oc, nullptr), "video recorder: Failed to write header");

    av_image_fill_arrays(
        this->d->src_frame_data, this->d->src_frame_linesize,
        image_view.data(), src_pix_fmt, width, height, 1
    );

    this->d->frame_convert_ctx = sws_getContext(
        width, height, src_pix_fmt,
        width, height, dst_pix_fmt,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    throw_if(!this->d->frame_convert_ctx, "Cannot initialize the conversion context");

    this->d->timestamp_height = std::min(height, original_timestamp_height);
    this->d->timestamp_convert_ctx = sws_getContext(
        width, this->d->timestamp_height, src_pix_fmt,
        width, this->d->timestamp_height, dst_pix_fmt,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    throw_if(!this->d->timestamp_convert_ctx, "Cannot initialize the conversion context");
}

static std::pair<int, char const*> encode_frame(
    AVFrame* picture, AVFormatContext* oc, AVCodecContext* codec_ctx,
    AVStream* video_st, AVPacket* pkt)
{
    int errnum = avcodec_send_frame(codec_ctx, picture);

    if (errnum < 0) {
        return {errnum, "Failed encoding a video frame"};
    }

    while (errnum >= 0) {
        errnum = avcodec_receive_packet(codec_ctx, pkt);

        if (errnum < 0) {
            if (errnum == AVERROR(EAGAIN) || errnum == AVERROR_EOF) {
                errnum = 0;
            }
            break;
        }

        av_packet_rescale_ts(pkt, codec_ctx->time_base, video_st->time_base);
        pkt->stream_index = video_st->index;
        /* Write the compressed frame to the media file. */
        errnum = av_interleaved_write_frame(oc, pkt);
        // av_packet_unref(&this->d->pkt);
    }

    return {errnum, "Failed while writing video frame"};
}

video_recorder::~video_recorder() /*NOLINT*/
{
    // flush the encoder
    auto [errnum, errmsg] = encode_frame(
        nullptr, this->d->oc, this->d->codec_ctx,
        this->d->video_st, this->d->pkt);

    check_errnum(errnum, errmsg);

    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
    av_write_trailer(this->d->oc);
}

void video_recorder::preparing_video_frame(int bottom)
{
    /* stat */// LOG(LOG_INFO, "preparing_video_frame");
    int height = std::max(bottom, this->d->previous_height);
    this->d->previous_height = bottom;
    sws_scale(
        this->d->frame_convert_ctx,
        this->d->src_frame_data,
        this->d->src_frame_linesize,
        0,
        std::min(height, this->d->original_height),
        this->d->dst_frame->data,
        this->d->dst_frame->linesize);
}

void video_recorder::preparing_video_frame()
{
    sws_scale(
        this->d->frame_convert_ctx,
        this->d->src_frame_data,
        this->d->src_frame_linesize,
        0,
        this->d->original_height,
        this->d->dst_frame->data,
        this->d->dst_frame->linesize);
}

void video_recorder::preparing_timestamp_video_frame()
{
    /* stat */// LOG(LOG_INFO, "preparing_timestamp_video_frame");
    sws_scale(
        this->d->timestamp_convert_ctx,
        this->d->src_frame_data,
        this->d->src_frame_linesize,
        0,
        this->d->timestamp_height,
        this->d->dst_frame->data,
        this->d->dst_frame->linesize);
}

void video_recorder::encoding_video_frame(int64_t frame_index)
{
    /* stat */// LOG(LOG_INFO, "encoding_video_frame %ld", frame_index);

    this->d->dst_frame->pts = frame_index;
    auto [errnum, errmsg] = encode_frame(
        this->d->dst_frame, this->d->oc, this->d->codec_ctx,
        this->d->video_st, this->d->pkt);

    check_errnum(errnum, errmsg);
}

#else

struct video_recorder::D {};

video_recorder::video_recorder(
    char const* /*filename*/, FilePermissions /*file_permissions*/, AclReportApi * /*acl_report*/,
    ImageView const & /*image_view*/, int /*frame_rate*/,
    const char * /*codec_name*/, char const* /*codec_options*/, int /*log_level*/
)
{}

video_recorder::~video_recorder() = default;

void video_recorder::preparing_video_frame(int) { }

void video_recorder::preparing_video_frame() { }

void video_recorder::preparing_timestamp_video_frame() { }

void video_recorder::encoding_video_frame(int64_t /*frame_index*/) { }

#endif
