#include "VideoContainer.h"
#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__)
#endif

VideoContainer::VideoContainer(const std::string& filename) {
    openContainer(filename);
    findStreamInfo();
    initCodecs();
    if (video_stream_index_ < 0) {
        throw std::runtime_error("No video stream found");
    }
}

void VideoContainer::openContainer(const std::string& filename) {
    format_ctx_ = avformat_alloc_context();
    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to open video file: " + filename);
    }
}

void VideoContainer::findStreamInfo() {
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        throw std::runtime_error("Failed to find stream information in container");
    }
}

void VideoContainer::initCodecs() {
    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
        AVCodecParameters* codec_params = format_ctx_->streams[i]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);

        if (codec == nullptr) {
            continue;
        }

        if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index_ < 0) {
            initVideoCodec(codec_params, i, codec);

        } else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0) {
            initAudioCodec(codec_params, i, codec);
        }
    }
}

void VideoContainer::initVideoCodec(AVCodecParameters* codec_params, int index,
                                    const AVCodec* codec) {

    video_stream_index_ = index;
    video_codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(video_codec_ctx_, codec_params);

    if (avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
        throw std::runtime_error("Failed to open video codec");
    }

    LOG("Opened video codec: %s (%s)", codec->name, codec->long_name ? codec->long_name : "?");
}

void VideoContainer::initAudioCodec(AVCodecParameters* codec_params, int index,
                                    const AVCodec* codec) {

    audio_stream_index_ = index;
    audio_codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(audio_codec_ctx_, codec_params);

    if (avcodec_open2(audio_codec_ctx_, codec, nullptr) < 0) {
        throw std::runtime_error("Failed to open audio codec");
    }

    LOG("Opened audio codec: %s (%s)", codec->name, codec->long_name ? codec->long_name : "?");
}
double VideoContainer::getDuration() const {
    if (format_ctx_) {
        return static_cast<double>(format_ctx_->duration) / AV_TIME_BASE;
    }
    return 0.0;
}

double VideoContainer::getWidth() const {
    return video_codec_ctx_->width;
}

double VideoContainer::getHeight() const {
    return video_codec_ctx_->height;
}
AVPixelFormat VideoContainer::getPixelFromat() const {
    return video_codec_ctx_->pix_fmt;
}

int VideoContainer::getNumberOfChannels() const {
    return audio_codec_ctx_->ch_layout.nb_channels;
}
double VideoContainer::audioTimeBase() const {
    double base = av_q2d(format_ctx_->streams[audio_stream_index_]->time_base);
    return base;
}
double VideoContainer::videoTimeBase() const {
    double base = av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
    return base;
}
int VideoContainer::channelLayout() const {
    return audio_codec_ctx_->ch_layout.u.mask;
}
int VideoContainer::sampleRate() const {
    return audio_codec_ctx_->sample_rate;
}
AVSampleFormat VideoContainer::sampleFormat() const {
    return audio_codec_ctx_->sample_fmt;
}

AVFormatContext* VideoContainer::getFormatContext() const {
    return format_ctx_;
}

int VideoContainer::getVideoStreamIndex() const {
    return video_stream_index_;
}
int VideoContainer::getAudioStreamIndex() const {
    return audio_stream_index_;
}

AVCodecContext* VideoContainer::getVideoCodecContext() const {
    return video_codec_ctx_;
}
AVCodecContext* VideoContainer::getAudioCodecContext() const {
    return audio_codec_ctx_;
}

AVRational VideoContainer::averageFrameRate() const {
    return format_ctx_->streams[video_stream_index_]->avg_frame_rate;
}
