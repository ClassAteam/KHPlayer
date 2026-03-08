#include "VideoPlayer.h"
#include "utility.h"
#include <chrono>
#include <iostream>
#include <thread>

VideoPlayer::VideoPlayer(const std::string& filename)
    : decoder_(Decoder(filename)), renderer_(decoder_) {}

void VideoPlayer::test() {
    std::thread decoding(&Decoder::decode, &decoder_);
    std::thread receive(&Renderer::recieveImages, &renderer_);
    while (!decoder_.isDecodingComplete()) {

        renderer_.renderFrame();
    }
}

VideoPlayer::~VideoPlayer() {}
