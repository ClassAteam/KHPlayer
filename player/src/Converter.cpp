#include "Converter.h"
#include "utility.h"
#include <iostream>
#include <ostream>
extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

Converter::Converter(Decoder& decoder) : decoder_(decoder) {
    frame_width_ = decoder.getContainer().getWidth();
    frame_height_ = decoder.getContainer().getHeight();
}

Converter::~Converter() {
    if (scaler_ctx_)
        sws_freeContext(scaler_ctx_);
}

void Converter::convert(std::atomic<bool>& quit, std::atomic<bool>& /*paused*/) {
    while (!quit) {

        {
            auto frame = decoder_.getFrame();
            if (!frame)
                break;

            auto recieved_frame = &frame.value();

            auto vf = convertFrame(recieved_frame);

            if (!vf) {
                av_frame_free(&recieved_frame->frame);
                continue;
            }

            if (!display_queue_.push(vf.value())) {
                av_frame_free(&vf->frame);
                av_frame_free(&recieved_frame->frame);
                break;
            }
            av_frame_free(&recieved_frame->frame);
        }
    }
}

std::optional<VideoFrame> Converter::convertFrame(Frame* frame) {
    // Fast path: YUV420P or NV12 — clone and pass through, no conversion
    auto fmt = static_cast<AVPixelFormat>(frame->frame->format);
    if (fmt == AV_PIX_FMT_YUV420P || fmt == AV_PIX_FMT_NV12 || fmt == AV_PIX_FMT_MEDIACODEC) {
        AVFrame* clone = av_frame_clone(frame->frame);
        if (!clone) {
            std::cerr << "av_frame_clone failed" << std::endl;
            return std::nullopt;
        }
        return VideoFrame{clone, frame->pts};
    }

    // Slow path: convert to NV12 (10-bit, YUYV, etc.)
    if (!scaler_ctx_) {
        TIMING_LOG("[converter] slow path: format=%d, creating scaler\n", fmt);
        scaler_ctx_ = sws_getContext(frame_width_, frame_height_, fmt,
                                     frame_width_, frame_height_,
                                     AV_PIX_FMT_NV12, SWS_FAST_BILINEAR,
                                     nullptr, nullptr, nullptr);
        if (!scaler_ctx_) {
            std::cerr << "sws_getContext failed" << std::endl;
            return std::nullopt;
        }
    }

    AVFrame* yuv = av_frame_alloc();
    yuv->format = AV_PIX_FMT_NV12;
    yuv->width = frame_width_;
    yuv->height = frame_height_;

    int ret = av_frame_get_buffer(yuv, 32);
    if (ret < 0) {
        std::cerr << "Failed to allocate YUV frame buffer: " << ret << std::endl;
        av_frame_free(&yuv);
        return std::nullopt;
    }

    static int    scale_count = 0;
    static double scale_sum   = 0;
    double scale_t0 = av_gettime() / 1e6;
    int height_ret = sws_scale(scaler_ctx_, frame->frame->data, frame->frame->linesize, 0,
                               frame_height_, yuv->data, yuv->linesize);
    scale_sum += av_gettime() / 1e6 - scale_t0;
    if (++scale_count >= 30) {
        TIMING_LOG("[converter] sws_scale avg=%.1fms\n", scale_sum / 30 * 1000.0);
        scale_count = 0; scale_sum = 0;
    }

    if (height_ret != frame_height_) {
        std::cerr << "sws_scale failed: expected " << frame_height_ << " but got " << height_ret
                  << std::endl;
        av_frame_free(&yuv);
        return std::nullopt;
    }

    return VideoFrame{yuv, frame->pts};
}

std::optional<VideoFrame> Converter::getImage() {
    return display_queue_.try_pop();
}

void Converter::close() {
    display_queue_.close();
}
