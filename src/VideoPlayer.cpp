#include "VideoPlayer.h"
#include "SdlContext.h"
#include "utility.h"
#include <chrono>
#include <iostream>
#include <thread>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

VideoPlayer::VideoPlayer(const std::string& filename)
    : decoder_(filename), sdl_context_(decoder_.getContainer()), converter_(decoder_, sdl_context_),
      renderer_(converter_, sdl_context_, quit_, paused_) {}

void VideoPlayer::test() {
    std::thread decoding([this]() { decoder_.decode(quit_); });
    std::thread receive([this]() { converter_.convert(quit_, paused_); });
    std::thread audio_feed([this]() {
        while (!quit_) {
            if (paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = decoder_.getAudioFrame(); // blocks until frame or closed
            if (!frame)
                break; // nullopt = decoder done
            sdl_context_.pushAudioFrame(*frame);
        }
    });

    renderer_.renderFrame();
    // quit_ is now true; unblock threads that may be blocked on full queues
    decoder_.closeQueues();
    converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}

void VideoPlayer::test_loop() {
    std::thread decoding([this]() { decoder_.decode(quit_, true); });
    std::thread receive([this]() { converter_.convert(quit_, paused_); });
    std::thread audio_feed([this]() {
        while (!quit_) {
            if (paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = decoder_.getAudioFrame(); // blocks until frame or closed
            if (!frame)
                break; // nullopt = decoder done
            sdl_context_.pushAudioFrame(*frame);
        }
    });

    renderer_.renderFrame();
    // quit_ is now true; unblock threads that may be blocked on full queues
    decoder_.closeQueues();
    converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}

VideoPlayer::~VideoPlayer() {}
