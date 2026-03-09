#include "VideoPlayer.h"
#include "SdlContext.h"
#include "utility.h"
#include <chrono>
#include <iostream>
#include <thread>

VideoPlayer::VideoPlayer(const std::string& filename)
    : decoder_(filename), sdl_context_(decoder_.getContainer()),
      converter_(decoder_, sdl_context_),
      renderer_(converter_, sdl_context_, quit_, paused_) {}

void VideoPlayer::test() {
    std::thread decoding([this]() { decoder_.decode(quit_); });
    std::thread receive([this]() { converter_.convert(quit_, paused_); });
    std::thread audio_feed([this]() {
        while (!quit_ && (!decoder_.isDecodingComplete() || !decoder_.isAudioQueueEmpty())) {
            if (paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = decoder_.getAudioFrame();
            if (frame)
                sdl_context_.pushAudioFrame(*frame);
        }
    });

    renderer_.renderFrame();

    decoding.join();
    receive.join();
    audio_feed.join();
}

VideoPlayer::~VideoPlayer() {}
