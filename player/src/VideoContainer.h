#pragma once
#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoContainer {
public:
    VideoContainer(const std::string& filename);
    ~VideoContainer();
    // Phase 2 init: open codecs with optional MediaCodec surface
    // Must be called after SdlContext sets up the GL surface.
    void openCodecs(void* surface = nullptr);

    double getDuration() const;
    double getWidth() const;
    double getHeight() const;
    AVFormatContext* getFormatContext() const;
    AVPixelFormat getPixelFromat() const;
    int getNumberOfChannels() const;
    double audioTimeBase() const;
    double videoTimeBase() const;
    AVChannelLayout channelLayout() const;
    int sampleRate() const;
    AVSampleFormat sampleFormat() const;
    bool hasAudio() const;
    int getVideoStreamIndex() const;
    int getAudioStreamIndex() const;
    AVCodecContext* getVideoCodecContext() const;
    AVCodecContext* getAudioCodecContext() const;
    AVRational averageFrameRate() const;

private:
    void openContainer(const std::string& filename);
    void findStreamInfo();
    void allocCodecContexts();
    void allocVideoCodecContext(AVCodecParameters* params, int index, const AVCodec* codec);
    void allocAudioCodecContext(AVCodecParameters* params, int index, const AVCodec* codec);

    AVFormatContext* format_ctx_;
    int video_stream_index_{-1};
    int audio_stream_index_{-1};
    AVCodecContext* video_codec_ctx_{nullptr};
    AVCodecContext* audio_codec_ctx_{nullptr};
    const AVCodec* video_codec_{nullptr};
    const AVCodec* audio_codec_{nullptr};

};
