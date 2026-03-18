#pragma once
#include "Converter.h"
#include <SDL_render.h>
#include <SdlContext.h>
#include <atomic>
extern "C" {
#include <libavutil/rational.h>
}

class Renderer {
public:
    Renderer(Converter& converter, SdlContext& sdl_context, std::atomic<bool>& quit,
             std::atomic<bool>& paused, AVRational avg_frame_rate);
    void renderFrame();

private:
    void delay(double pts);
    SDL_Texture* texture_;
    SDL_Renderer* renderer_;
    Converter& converter_;
    SdlContext& sdl_context_;
    std::atomic<bool>& quit_;
    std::atomic<bool>& paused_;

    double frame_last_pts_;
    double frame_last_delay_;
    double frame_timer_;
};
