#pragma once
#include "VideoContainer.h"
#include <mutex>
#include <optional>
#include <queue>

struct Frame {
    AVFrame* frame;
    double pts;
};
class Decoder {
public:
    Decoder(const std::string& filename);
    void decode();
    void inspect_last_frame();
    std::optional<Frame> getFrame();
    VideoContainer& getContainer();
    bool isDecodingComplete();
    bool isQueueEmpty();

private:
    void decodeVideoStream();
    void decodeAudioStream();
    VideoContainer container_;

    std::queue<Frame> video_queue_;
    std::queue<AVFrame*> audio_queue_;
    AVPacket* packet_;
    AVFrame* frame_;
    int64_t video_frame_count_{0};

    static const int MAX_QUEUE_SIZE = 25;
    bool decoding_complete_{false};
};
