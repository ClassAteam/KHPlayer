#include "Renderer.h"
#include "VideoContainer.h"
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <ostream>
#include <thread>
extern "C" {
#include <libavutil/time.h>
}

Renderer::Renderer(Converter& converter, SdlContext& sdl_context, std::atomic<bool>& quit,
                   std::atomic<bool>& paused)
    : converter_(converter), sdl_context_(sdl_context), quit_(quit), paused_(paused) {

    texture_ = sdl_context.getTexture();
    renderer_ = sdl_context.getRenderer();
    frame_last_pts_ = 0.0;
    frame_last_delay_ = 1.0 / 25.0;
}

void Renderer::renderFrame() {
    frame_timer_ = static_cast<double>(av_gettime()) / 1000000.0;
    while (!quit_) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit_ = true;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                paused_ = !paused_;
                sdl_context_.pauseAudio(paused_);
            }
        }

        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

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

    double audio_clock = sdl_context_.getAudioClock();
    double diff = pts - audio_clock;
    double sync_threshold = std::max(delay, 0.01);
    if (fabs(diff) < 10.0 && fabs(diff) > sync_threshold) {
        double max_step = delay * 0.5;
        double correction = std::max(-max_step, std::min(max_step, diff));
        delay = std::max(0.0, delay + correction);
    }

    frame_timer_ += delay;
    double actual_delay = frame_timer_ - (static_cast<double>(av_gettime()) / 1000000.0);
    if (actual_delay > 0.0 && actual_delay < 1.0)
        av_usleep(static_cast<unsigned int>(actual_delay * 1000000.0));
}
