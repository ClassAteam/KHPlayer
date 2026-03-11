#pragma once
#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoContainer {
public:
    VideoContainer(const std::string& filename);
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
    int getVideoStreamIndex() const;
    int getAudioStreamIndex() const;
    AVCodecContext* getVideoCodecContext() const;
    AVCodecContext* getAudioCodecContext() const;
    AVRational averageFrameRate() const;

private:
    void openContainer(const std::string& filename);
    void findStreamInfo();
    void initCodecs();
    void initVideoCodec(AVCodecParameters* codec_params, int index, const AVCodec* codec);
    void initAudioCodec(AVCodecParameters* codec_params, int index, const AVCodec* codec);
    AVFormatContext* format_ctx_;
    int video_stream_index_{-1};
    int audio_stream_index_{-1};
    AVCodecContext* video_codec_ctx_;
    AVCodecContext* audio_codec_ctx_;
};
