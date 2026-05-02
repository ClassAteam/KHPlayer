#pragma once
#include "Converter.h"
#include <SDL_render.h>
#include <SdlContext.h>
#include <atomic>
extern "C" {
#include <libavutil/rational.h>
}

struct FrameStats {
    double wall_interval{0};   // actual wall time between frames
    double av_diff{0};         // pts - audio_clock
    double sleep_requested{0}; // actual_delay passed to av_usleep
    double sdl_ms{0};          // SDL_UpdateTexture + SDL_RenderPresent
    double pts_diff{0};        // pts - frame_last_pts (pre-correction delay)
};

class Renderer {
public:
    Renderer(Converter& converter, SdlContext& sdl_context, std::atomic<bool>& quit,
             std::atomic<bool>& paused, AVRational avg_frame_rate);
    void renderFrame();

private:
    void delay(double pts, FrameStats& stats);
    void printStats(const FrameStats& stats);

    SDL_Renderer* renderer_;
    Converter& converter_;
    SdlContext& sdl_context_;
    std::atomic<bool>& quit_;
    std::atomic<bool>& paused_;

    double frame_last_pts_;
    double frame_last_delay_;
    double frame_timer_;

    // timing accumulators, reset every kStatWindow frames
    static constexpr int kStatWindow = 30;
    int stat_frame_{0};
    double sum_wall_{0}, max_wall_{0};
    double sum_diff_{0};
    double sum_sleep_{0}, max_sleep_{0};
    double sum_sdl_{0};
    double sum_pts_diff_{0};
    double last_frame_wall_{0};
};
