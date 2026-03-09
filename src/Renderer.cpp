#include "Renderer.h"
#include "VideoContainer.h"
#include <iostream>
#include <optional>
#include <ostream>
extern "C" {
#include <libavutil/time.h>
}

Renderer::Renderer(Converter& converter, SdlContext& sdl_context) : converter_(converter) {

    texture_ = sdl_context.getTexture();
    renderer_ = sdl_context.getRenderer();
    frame_last_pts_ = 0.0;
    frame_last_delay_ = 1.0 / 25.0;
}

void Renderer::renderFrame() {
    frame_timer_ = static_cast<double>(av_gettime()) / 1000000.0;
    while (true) {

        auto result = converter_.getImage();
        if (!result)
            continue;

        auto image = result.value();

        if (SDL_UpdateTexture(texture_, nullptr, image.bgra->data[0], image.bgra->linesize[0]) < 0)
            std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << std::endl;
        if (SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255) < 0)
            std::cerr << "SDL_SetRenderDrawColor failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderClear(renderer_) < 0)
            std::cerr << "SDL_RenderClear failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderCopy(renderer_, texture_, nullptr, nullptr) < 0)
            std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << std::endl;
        SDL_RenderPresent(renderer_);
        delay(image.pts);

        av_frame_free(&image.bgra);
    }
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
