#pragma once
#include "Converter.h"
#include "Decoder.h"
#include "Renderer.h"
#include "SdlContext.h"
#include <atomic>
#include <optional>
#include <string>

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    VideoPlayer(const std::string& filename, SdlContext& ctx);
    void test();
    void test_loop();
    SdlContext& getSdlContext();
    ~VideoPlayer();

private:
    std::atomic<bool> quit_{false};
    std::atomic<bool> paused_{false};
    Decoder decoder_;
    std::optional<SdlContext> sdl_context_owned_;
    SdlContext* sdl_context_;
    Converter converter_;
    Renderer renderer_;
};
