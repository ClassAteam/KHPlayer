#pragma once
#include "Decoder.h"
#include "Renderer.h"

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    void test();
    ~VideoPlayer();

private:
    Decoder decoder_;
    Renderer renderer_;
};
