#include "Converter.h"
#include <chrono>
#include <iostream>
#include <ostream>
#include <thread>
Converter::Converter(Decoder& decoder, SdlContext& sdl_context) : decoder_(decoder) {

    frame_width_ = decoder.getContainer().getWidth();
    frame_height_ = decoder.getContainer().getHeight();
    scaler_ctx_ = sdl_context.getScalerContext();
}

void Converter::convert(std::atomic<bool>& quit, std::atomic<bool>& paused) {
    while (!quit && (!decoder_.isDecodingComplete() || !decoder_.isQueueEmpty())) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        auto frame = decoder_.getFrame();
        if (!frame) {
            continue;
        }
        auto recieved_frame = &frame.value();

        auto bgra_frame = convertFrame(recieved_frame);

        if (!bgra_frame)
            continue;

        display_queue_.push(bgra_frame.value());
        av_frame_free(&recieved_frame->frame);
        bgra_frame_count_++;
    }
}
std::optional<BgraFrame> Converter::convertFrame(Frame* frame) {

    BgraFrame new_frame;
    new_frame.bgra = av_frame_alloc();
    new_frame.bgra->format = AV_PIX_FMT_BGRA;
    new_frame.bgra->width = frame_width_;
    new_frame.bgra->height = frame_height_;

    int ret = av_frame_get_buffer(new_frame.bgra, 32);
    if (ret < 0) {
        std::cerr << "Failed to allocate BGRA frame buffer: " << ret << std::endl;
        av_frame_free(&new_frame.bgra);
        return std::nullopt;
    }

    // Convert frame to BGRA format
    int height_ret = sws_scale(scaler_ctx_, frame->frame->data, frame->frame->linesize, 0,
                               frame_height_, new_frame.bgra->data, new_frame.bgra->linesize);
    if (height_ret != frame_height_) {
        std::cerr << "sws_scale failed: expected " << frame_height_ << " but got " << height_ret
                  << std::endl;
        av_frame_free(&new_frame.bgra);
        return std::nullopt;
    }
    if (!new_frame.bgra->data[0]) {
        std::cerr << "Frame data is NULL after conversion" << std::endl;
        av_frame_free(&new_frame.bgra);
        return std::nullopt;
    }

    new_frame.pts = frame->pts;
    return new_frame;
}

std::optional<BgraFrame> Converter::getImage() {
    if (display_queue_.empty())
        return std::nullopt;
    auto image = display_queue_.front();
    display_queue_.pop();
    return image;
}
