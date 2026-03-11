#include "SdlContext.h"
#include <SDL.h>
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}
#include <mutex>
#include <stdexcept>

SdlContext::SdlContext(const VideoContainer& container)
    : sws_ctx_(nullptr), swr_ctx_(nullptr), window_(nullptr), renderer_(nullptr), texture_(nullptr),
      audio_device_(0) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
        ;
    }
    initWindow(container.getWidth(), container.getHeight());
    initRenderer();
    initAudioDevice(container.sampleRate(), container.getNumberOfChannels());
    createTexture(container.getWidth(), container.getHeight());
    SDL_RaiseWindow(window_);
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
    createScaler(container.getWidth(), container.getHeight(), container.getPixelFromat());
    number_of_channels_ = container.getNumberOfChannels();
    time_base_ = container.audioTimeBase();
    initResamplerContext(container.channelLayout(), container.getNumberOfChannels(),
                         container.sampleFormat());
}

void SdlContext::initWindow(int width, int height) {

    window_ =
        SDL_CreateWindow("Simple Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }
}
void SdlContext::initRenderer() {

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        throw std::runtime_error(std::string("Failed to create renderer: ") + SDL_GetError());
    }
}
void SdlContext::createTexture(int width, int height) {

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 width, height);

    if (!texture_) {
        throw std::runtime_error(std::string("Failed to create texture: ") + SDL_GetError());
    }
}
void SdlContext::createScaler(int width, int height, AVPixelFormat format) {

    sws_ctx_ = sws_getContext(width, height, format, width, height, AV_PIX_FMT_BGRA, SWS_BILINEAR,
                              nullptr, nullptr, nullptr);
}
void SdlContext::sdlAudioCallback(void* userdata, uint8_t* stream, int len) {
    SdlContext* sdl_context = static_cast<SdlContext*>(userdata);
    sdl_context->audioCallback(stream, len);
}

void SdlContext::audioCallback(uint8_t* stream, int len) {
    SDL_memset(stream, 0, len);

    std::lock_guard<std::mutex> lock(audio_mutex_);

    if (audio_queue_.empty()) {
        return;
    }

    AVFrame* frame = audio_queue_.front();
    audio_queue_.pop();

    int out_buffer_size = av_samples_get_buffer_size(nullptr, number_of_channels_,
                                                     frame->nb_samples, AV_SAMPLE_FMT_S16, 1);

    uint8_t* output = (uint8_t*)av_malloc(out_buffer_size);
    if (!output) {
        av_frame_free(&frame);
        return;
    }

    uint8_t* out_ptr = output;
    int out_samples = swr_convert(swr_ctx_, &out_ptr, frame->nb_samples,
                                  (const uint8_t**)frame->data, frame->nb_samples);

    if (out_samples > 0) {
        int data_size = out_samples * number_of_channels_ * sizeof(int16_t);
        int copy_size = (data_size < len) ? data_size : len;
        SDL_MixAudioFormat(stream, output, AUDIO_S16SYS, copy_size, SDL_MIX_MAXVOLUME);

        double pts = frame->pts * time_base_;
        audio_clock_ = pts;
    }

    av_free(output);
    av_frame_free(&frame);
}
void SdlContext::initAudioDevice(int sample_rate, int num_of_channels) {

    SDL_AudioSpec wanted_spec, obtained_spec;
    wanted_spec.freq = sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = num_of_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = sdlAudioCallback;
    wanted_spec.userdata = this;
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, &obtained_spec, 0);
    if (audio_device_ == 0) {
        throw std::runtime_error(std::string("Failed to open audio device: ") + SDL_GetError());
    }
    SDL_PauseAudioDevice(audio_device_, 0);
}

void SdlContext::initResamplerContext(AVChannelLayout ch_layout, int sample_rate, AVSampleFormat fmt) {

    swr_alloc_set_opts2(&swr_ctx_, &ch_layout, AV_SAMPLE_FMT_S16, sample_rate,
                        &ch_layout, fmt, sample_rate, 0, nullptr);
    swr_init(swr_ctx_);
}

void SdlContext::pushAudioFrame(AVFrame* frame) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_queue_.push(frame);
}

size_t SdlContext::audioQueueSize() const {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    return audio_queue_.size();
}

SwsContext* SdlContext::getScalerContext() {
    return sws_ctx_;
}

SDL_Renderer* SdlContext::getRenderer() {
    return renderer_;
}

SDL_Texture* SdlContext::getTexture() {
    return texture_;
}

double SdlContext::getAudioClock() const {
    return audio_clock_;
}

void SdlContext::pauseAudio(bool paused) {
    SDL_PauseAudioDevice(audio_device_, paused ? 1 : 0);
}
