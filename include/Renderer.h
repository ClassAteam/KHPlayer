#include "Converter.h"
#include <SDL_render.h>
#include <SdlContext.h>

class Renderer {
public:
    Renderer(Converter& converter, SdlContext& sdl_context);
    void renderFrame();

private:
    void delay(double pts);
    SDL_Texture* texture_;
    SDL_Renderer* renderer_;
    Converter& converter_;

    double frame_last_pts_;
    double frame_last_delay_;
    double audio_clock_;
    double frame_timer_;
    double current_time_;
};
