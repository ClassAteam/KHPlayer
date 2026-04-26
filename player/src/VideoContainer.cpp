#include "VideoContainer.h"
#include "logging.h"
#include <stdexcept>
extern "C" {
#include <libavcodec/mediacodec.h>
}

VideoContainer::VideoContainer(const std::string& filename) {
    openContainer(filename);
    findStreamInfo();
    allocCodecContexts();
}

VideoContainer::~VideoContainer() {
#ifdef ANDROID
    if (video_codec_ctx_)
        av_mediacodec_default_free(video_codec_ctx_);
#endif
    avcodec_free_context(&video_codec_ctx_);
    avcodec_free_context(&audio_codec_ctx_);
    avformat_close_input(&format_ctx_);
}

void VideoContainer::openCodecs(void* surface) {
    if (video_stream_index_ < 0)
        throw std::runtime_error("No video stream found");

#ifdef ANDROID
    if (surface) {
        AVMediaCodecContext* mc_ctx = av_mediacodec_alloc_context();
        if (!mc_ctx)
            throw std::runtime_error("Failed to allocate AVMediaCodecContext");

        LOG("openCodecs: calling av_mediacodec_default_init");
        av_mediacodec_default_init(video_codec_ctx_, mc_ctx, surface);
        // avcodec_default_get_format ignores hwaccel_context (AD_HOC method),
        // so AV_PIX_FMT_MEDIACODEC would never be selected without this callback.
        video_codec_ctx_->get_format = [](AVCodecContext* ctx,
                                          const AVPixelFormat* fmts) -> AVPixelFormat {
            for (int i = 0; fmts[i] != AV_PIX_FMT_NONE; i++) {
                if (fmts[i] == AV_PIX_FMT_MEDIACODEC)
                    return AV_PIX_FMT_MEDIACODEC;
            }
            return avcodec_default_get_format(ctx, fmts);
        };
    }
#endif

    // TODO we are hanging there now
    if (avcodec_open2(video_codec_ctx_, video_codec_, nullptr) < 0)
        throw std::runtime_error("Failed to open video codec");

    LOG("Opened video codec: %s (%s)", video_codec_->name,
        video_codec_->long_name ? video_codec_->long_name : "?");

    if (avcodec_open2(audio_codec_ctx_, audio_codec_, nullptr) < 0)
        throw std::runtime_error("Failed to open audio codec");
    LOG("Opened audio codec: %s (%s)", audio_codec_->name,
        audio_codec_->long_name ? audio_codec_->long_name : "?");
}

void VideoContainer::openContainer(const std::string& filename) {
    format_ctx_ = avformat_alloc_context();
    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Failed to open video file: " + filename);
}

void VideoContainer::findStreamInfo() {
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0)
        throw std::runtime_error("Failed to find stream information in container");
}

void VideoContainer::allocCodecContexts() {
    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
        AVCodecParameters* params = format_ctx_->streams[i]->codecpar;

        // TODO don't forget to choose MediaCodec forcefully for
        // heavy codecs like we did in  3faaf3a05467755ec683f74f6abc5f5d9c4114ee
        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (!codec)
            continue;

        if (params->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index_ < 0) {
            allocVideoCodecContext(params, i, codec);
        } else if (params->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0)
            allocAudioCodecContext(params, i, codec);
    }
}

void VideoContainer::allocVideoCodecContext(AVCodecParameters* params, int index,
                                            const AVCodec* codec) {
    video_stream_index_ = index;
    video_codec_ = codec;
    video_codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(video_codec_ctx_, params);
    video_codec_ctx_->pkt_timebase = format_ctx_->streams[index]->time_base;
}

void VideoContainer::allocAudioCodecContext(AVCodecParameters* params, int index,
                                            const AVCodec* codec) {
    audio_stream_index_ = index;
    audio_codec_ = codec;
    audio_codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(audio_codec_ctx_, params);
}

double VideoContainer::getDuration() const {
    if (format_ctx_)
        return static_cast<double>(format_ctx_->duration) / AV_TIME_BASE;
    return 0.0;
}

double VideoContainer::getWidth() const {
    return format_ctx_->streams[video_stream_index_]->codecpar->width;
}

double VideoContainer::getHeight() const {
    return format_ctx_->streams[video_stream_index_]->codecpar->height;
}

AVPixelFormat VideoContainer::getPixelFromat() const {
    return video_codec_ctx_->pix_fmt;
}

int VideoContainer::getNumberOfChannels() const {
    return audio_codec_ctx_->ch_layout.nb_channels;
}
double VideoContainer::audioTimeBase() const {
    return av_q2d(format_ctx_->streams[audio_stream_index_]->time_base);
}
double VideoContainer::videoTimeBase() const {
    return av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
}
AVChannelLayout VideoContainer::channelLayout() const {
    return audio_codec_ctx_->ch_layout;
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
