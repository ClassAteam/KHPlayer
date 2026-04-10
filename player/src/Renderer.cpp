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
#include <cstdio>
#include <iostream>
#include <optional>
#include <ostream>
#include <thread>
extern "C" {
#include <libavutil/time.h>
}
#ifdef __ANDROID__
#include <android/log.h>
#define TIMING_LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#define TIMING_LOG(...) fprintf(stderr, __VA_ARGS__)
#endif

Renderer::Renderer(Converter& converter, SdlContext& sdl_context, std::atomic<bool>& quit,
                   std::atomic<bool>& paused, AVRational avg_frame_rate)
    : converter_(converter), sdl_context_(sdl_context), quit_(quit), paused_(paused) {

    texture_ = sdl_context.getTexture();
    renderer_ = sdl_context.getRenderer();
    frame_last_pts_ = 0.0;
    frame_last_delay_ = (avg_frame_rate.num > 0) ? av_q2d(av_inv_q(avg_frame_rate)) : 1.0 / 25.0;
}

void Renderer::renderFrame() {
    TIMING_LOG("Renderer::renderFrame() started\n");
    bool first_frame = true;
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

        double frame_start = static_cast<double>(av_gettime()) / 1000000.0;

        if (first_frame) {
            frame_timer_ = frame_start;
            first_frame = false;
        }

        if (SDL_UpdateTexture(texture_, nullptr, image.bgra->data[0], image.bgra->linesize[0]) < 0)
            std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << std::endl;
        if (SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255) < 0)
            std::cerr << "SDL_SetRenderDrawColor failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderClear(renderer_) < 0)
            std::cerr << "SDL_RenderClear failed: " << SDL_GetError() << std::endl;
        if (SDL_RenderCopy(renderer_, texture_, nullptr, nullptr) < 0)
            std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << std::endl;
        SDL_RenderPresent(renderer_);

        FrameStats stats;
        stats.wall_interval = frame_start - last_frame_wall_;
        last_frame_wall_ = frame_start;
        delay(image.pts, stats);
        printStats(stats);

        av_frame_free(&image.bgra);
    }
}

void Renderer::printStats(const FrameStats& s) {
    if (stat_frame_ == 0) {
        // skip first frame — wall_interval is meaningless (was 0 at init)
        stat_frame_++;
        return;
    }
    sum_wall_  += s.wall_interval;
    max_wall_   = std::max(max_wall_,  s.wall_interval);
    sum_diff_  += std::fabs(s.av_diff);
    sum_sleep_ += s.sleep_requested;
    max_sleep_  = std::max(max_sleep_, s.sleep_requested);

    if (++stat_frame_ >= kStatWindow) {
        double n = kStatWindow - 1; // exclude first skipped frame
        TIMING_LOG(
                "[timing] wall avg=%.1fms max=%.1fms | sleep avg=%.1fms max=%.1fms | |av_diff| avg=%.1fms\n",
                sum_wall_  / n * 1000.0,
                max_wall_       * 1000.0,
                sum_sleep_ / n * 1000.0,
                max_sleep_      * 1000.0,
                sum_diff_  / n * 1000.0);
        stat_frame_ = 0;
        sum_wall_ = max_wall_ = sum_diff_ = sum_sleep_ = max_sleep_ = 0;
    }
}

void Renderer::delay(double pts, FrameStats& stats) {
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

    stats.av_diff = diff;
    stats.sleep_requested = (actual_delay > 0.0 && actual_delay < 1.0) ? actual_delay : 0.0;
}
