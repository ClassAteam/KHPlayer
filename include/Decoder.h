#pragma once
#include "VideoContainer.h"
#include <atomic>
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
    void decode(std::atomic<bool>& quit);
    void inspect_last_frame();
    std::optional<Frame> getFrame();
    std::optional<AVFrame*> getAudioFrame();
    VideoContainer& getContainer();
    bool isDecodingComplete();
    bool isQueueEmpty();
    bool isAudioQueueEmpty();

private:
    void decodeVideoPacket();
    void decodeAudioPacket();
    VideoContainer container_;

    std::queue<Frame> video_queue_;
    std::queue<AVFrame*> audio_queue_;
    AVPacket* packet_;
    AVFrame* frame_;
    int64_t video_frame_count_{0};

    static const int MAX_QUEUE_SIZE = 25;
    bool decoding_complete_{false};
};
