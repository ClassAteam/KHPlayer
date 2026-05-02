#include "VideoPlayer.h"
#include "logging.h"
#include <chrono>
#include <thread>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif
#ifdef ANDROID
#include <SDL_system.h>
#include <jni.h>
extern "C" {
#include <libavcodec/jni.h>
}
#endif

static void android_setup_jni(SdlContext& ctx, VideoContainer& container) {
#ifdef ANDROID
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    JavaVM* vm = nullptr;
    env->GetJavaVM(&vm);
    av_jni_set_java_vm(vm, nullptr);
    void* surface = ctx.setupGLSurfaceRenderer();
    LOG("openCodecs: entering");
    container.openCodecs(surface);
#else
    container.openCodecs(nullptr);
#endif
    LOG("codec are opened");
}

VideoPlayer::VideoPlayer(const std::string& filename)
    : decoder_(filename),
      sdl_context_owned_(std::in_place, decoder_.getContainer()),
      sdl_context_(&*sdl_context_owned_),
      converter_(decoder_),
      renderer_(converter_, *sdl_context_, quit_, paused_,
                decoder_.getContainer().averageFrameRate()) {
    android_setup_jni(*sdl_context_, decoder_.getContainer());
}

VideoPlayer::VideoPlayer(const std::string& filename, SdlContext& ctx)
    : decoder_(filename),
      sdl_context_owned_(std::nullopt),
      sdl_context_(&ctx),
      converter_(decoder_),
      renderer_(converter_, *sdl_context_, quit_, paused_,
                decoder_.getContainer().averageFrameRate()) {
    LOG("player2: constructor body reached");
    sdl_context_->reinitForVideo(decoder_.getContainer());
    LOG("player2: reinitForVideo done");
    android_setup_jni(*sdl_context_, decoder_.getContainer());
    LOG("player2: android_setup_jni done");
}

VideoPlayer::~VideoPlayer() = default;

SdlContext& VideoPlayer::getSdlContext() {
    return *sdl_context_;
}

void VideoPlayer::test() {
    std::thread decoding([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Decoder");
#endif
        decoder_.decode(quit_);
    });
    std::thread receive([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Converter");
#endif
        converter_.convert(quit_, paused_);
    });
    std::thread audio_feed([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("AudioFeed");
#endif
        while (!quit_) {
            if (paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = decoder_.getAudioFrame();
            if (!frame)
                break;
            sdl_context_->pushAudioFrame(*frame);
        }
    });

    renderer_.renderFrame();
    decoder_.closeQueues();
    converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}

void VideoPlayer::test_loop() {
    std::thread decoding([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Decoder");
#endif
        decoder_.decode(quit_, false);
    });

    std::thread receive([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("Converter");
#endif
        converter_.convert(quit_, paused_);
    });

    std::thread audio_feed([this]() {
#ifdef TRACY_ENABLE
        tracy::SetThreadName("AudioFeed");
#endif
        while (!quit_) {
            if (paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            auto frame = decoder_.getAudioFrame();
            if (!frame)
                break;
            sdl_context_->pushAudioFrame(*frame);
        }
    });

    renderer_.renderFrame();
    decoder_.closeQueues();
    converter_.close();

    decoding.join();
    receive.join();
    audio_feed.join();
}
