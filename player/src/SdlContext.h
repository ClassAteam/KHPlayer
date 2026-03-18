#pragma once
#include "BoundedQueue.h"
#include "VideoContainer.h"
#include <SDL_audio.h>
#include <SDL_render.h>
#include <SDL_video.h>
extern "C" {
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <atomic>
#include <vector>

class SdlContext {
public:
    SdlContext(const VideoContainer& container);
    ~SdlContext();
    SwsContext* getScalerContext();
    SDL_Renderer* getRenderer();
    SDL_Texture* getTexture();
    void pushAudioFrame(AVFrame* frame);
    double getAudioClock() const;
    void pauseAudio(bool paused);

private:
    void initWindow(int widht, int height);
    void initRenderer();
    void createTexture(int widht, int height);
    void createScaler(int width, int height, AVPixelFormat format);
    void initAudioDevice(int sample_rate, int channels);
    static void sdlAudioCallback(void* userdata, uint8_t* stream, int len);
    void audioCallback(uint8_t* stream, int len);
    void initResamplerContext(AVChannelLayout ch_layout, int sample_rate, AVSampleFormat fmt);

    SwsContext* sws_ctx_;
    SwrContext* swr_ctx_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    SDL_AudioDeviceID audio_device_;
    BoundedQueue<AVFrame*> audio_queue_{25};
    int number_of_channels_;
    double time_base_;
    std::atomic<double> audio_clock_{0.0};
    std::vector<uint8_t> audio_buf_;
};
