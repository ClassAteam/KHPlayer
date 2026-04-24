#pragma once
#include "Decoder.h"
#include "SdlContext.h"
#include "Converter.h"
#include "Renderer.h"
#include <atomic>
#include <string>

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    void test();
    void test_loop();
    ~VideoPlayer();

private:
    std::atomic<bool> quit_{false};
    std::atomic<bool> paused_{false};
    Decoder decoder_;
    SdlContext sdl_context_;
    Converter converter_;
    Renderer renderer_;
};
