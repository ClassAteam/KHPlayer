#include "Renderer.h"
#include "VideoContainer.h"
#include <iostream>
#include <optional>
#include <ostream>
extern "C" {
#include <libavutil/time.h>
}

Renderer::Renderer(Decoder& decoder) : sdl_context_(decoder.getContainer()), decoder_(decoder) {

    frame_width_ = decoder_.getContainer().getWidth();
    frame_height_ = decoder_.getContainer().getHeight();
    scaler_ctx_ = sdl_context_.getScalerContext();
    frame_last_pts_ = 0.0;
    frame_last_delay_ = 1.0 / 25.0;
}

void Renderer::recieveImages() {
    while (!decoder_.isDecodingComplete() || !decoder_.isQueueEmpty()) {

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
    std::cout << "We finshed processing the frames, the total amount of images processd:"
              << bgra_frame_count_ << std::endl;
}

void Renderer::renderFrame() {
    frame_timer_ = static_cast<double>(av_gettime()) / 1000000.0;
    while (true) {

        if (display_queue_.empty()) {
            continue;
        }
        auto result = getImage();
        if (!result)
            continue;

        auto image = result.value();

        auto texture = sdl_context_.getTexture();
        auto renderer = sdl_context_.getRenderer();

        if (SDL_UpdateTexture(texture, nullptr, image.bgra->data[0], image.bgra->linesize[0]) < 0)
            std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << std::endl;
        if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) < 0)
            std::cerr << "SDL_SetRenderDrawColor failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderClear(renderer) < 0)
            std::cerr << "SDL_RenderClear failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) < 0)
            std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << std::endl;
        SDL_RenderPresent(renderer);
        delay(image.pts);

        av_frame_free(&image.bgra);
    }
}

std::optional<BgraFrame> Renderer::getImage() {
    if (display_queue_.empty())
        return std::nullopt;
    auto image = display_queue_.front();
    display_queue_.pop();
    return image;
}

std::optional<BgraFrame> Renderer::convertFrame(Frame* frame) {

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

void Renderer::delay(double pts) {
    double delay = pts - frame_last_pts_;
    if (delay <= 0 || delay >= 1.0)
        delay = frame_last_delay_;

    frame_last_delay_ = delay;
    frame_last_pts_ = pts;

    frame_timer_ += delay;
    double actual_delay = frame_timer_ - (static_cast<double>(av_gettime()) / 1000000.0);
    if (actual_delay > 0.0 && actual_delay < 1.0)
        av_usleep(static_cast<unsigned int>(actual_delay * 1000000.0));
}
