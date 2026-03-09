#pragma once
#include "Decoder.h"
#include "Renderer.h"
#include "SdlContext.h"
#include <atomic>

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    void test();
    ~VideoPlayer();

private:
    std::atomic<bool> quit_{false};
    std::atomic<bool> paused_{false};
    Decoder decoder_;
    SdlContext sdl_context_;
    Converter converter_;
    Renderer renderer_;
};
