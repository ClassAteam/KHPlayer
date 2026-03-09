#include "VideoPlayer.h"
#include "SdlContext.h"
#include "utility.h"
#include <chrono>
#include <iostream>
#include <thread>

VideoPlayer::VideoPlayer(const std::string& filename)
    : decoder_(Decoder(filename)), sdl_context_(SdlContext(decoder_.getContainer())),
      converter_(decoder_, sdl_context_), renderer_(converter_, sdl_context_) {}

void VideoPlayer::test() {
    std::thread decoding(&Decoder::decode, &decoder_);
    std::thread receive(&Converter::convert, &converter_);
    while (!decoder_.isDecodingComplete()) {

        renderer_.renderFrame();
    }
}

VideoPlayer::~VideoPlayer() {}
