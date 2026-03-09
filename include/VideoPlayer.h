#pragma once
#include "Decoder.h"
#include "Renderer.h"
#include "SdlContext.h"

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    void test();
    ~VideoPlayer();

private:
    Decoder decoder_;
    SdlContext sdl_context_;
    Converter converter_;
    Renderer renderer_;
};
