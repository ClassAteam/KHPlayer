#include "VideoPlayer.h"
#include "Converter.h"
#include "Decoder.h"
#include "Renderer.h"
#include "SdlContext.h"
#include "utility.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

struct VideoPlayer::Impl {
    std::atomic<bool> quit_{false};
    std::atomic<bool> paused_{false};
    Decoder decoder_;
    SdlContext sdl_context_;
    Converter converter_;
    Renderer renderer_;

    Impl(const std::string& filename)
        : decoder_(filename), sdl_context_(decoder_.getContainer()),
          converter_(decoder_, sdl_context_),
          renderer_(converter_, sdl_context_, quit_, paused_,
                    decoder_.getContainer().averageFrameRate()) {}
};

VideoPlayer::VideoPlayer(const std::string& filename)
    : impl_(std::make_unique<Impl>(filename)) {}

VideoPlayer::~VideoPlayer() = default;

void VideoPlayer::test() {
    std::thread decoding([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Decoder");
#endif
        impl_->decoder_.decode(impl_->quit_);
    });
    std::thread receive([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Converter");
#endif
        impl_->converter_.convert(impl_->quit_, impl_->paused_);
    });
    std::thread audio_feed([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("AudioFeed");
#endif
        while (!impl_->quit_) {
            if (impl_->paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = impl_->decoder_.getAudioFrame();
            if (!frame)
                break;
            impl_->sdl_context_.pushAudioFrame(*frame);
        }
    });

    impl_->renderer_.renderFrame();
    impl_->decoder_.closeQueues();
    impl_->converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}

void VideoPlayer::test_loop() {
    std::thread decoding([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Decoder");
#endif
        impl_->decoder_.decode(impl_->quit_, true);
    });

    std::thread receive([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Converter");
#endif
        impl_->converter_.convert(impl_->quit_, impl_->paused_);
    });

    std::thread audio_feed([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("AudioFeed");
#endif
        while (!impl_->quit_) {
            if (impl_->paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = impl_->decoder_.getAudioFrame();
            if (!frame)
                break;
            impl_->sdl_context_.pushAudioFrame(*frame);
        }
    });

    impl_->renderer_.renderFrame();
    impl_->decoder_.closeQueues();
    impl_->converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}
