#pragma once
#include "BoundedQueue.h"
#include "VideoContainer.h"
#include <atomic>
#include <optional>

struct Frame {
    AVFrame* frame;
    double pts;
};
class Decoder {
public:
    Decoder(const std::string& filename);
    void decode(std::atomic<bool>& quit, bool loop = false);
    void inspect_last_frame();
    std::optional<Frame> getFrame();
    std::optional<AVFrame*> getAudioFrame();
    VideoContainer& getContainer();
    bool isDecodingComplete();
    bool isQueueEmpty();
    bool isAudioQueueEmpty();
    void closeQueues();

private:
    void decodeVideoPacket();
    void decodeAudioPacket();
    VideoContainer container_;

    static const int MAX_QUEUE_SIZE = 25;
    BoundedQueue<Frame> video_queue_{MAX_QUEUE_SIZE};
    BoundedQueue<AVFrame*> audio_queue_{MAX_QUEUE_SIZE};
    AVPacket* packet_;
    AVFrame* frame_;
    int64_t video_frame_count_{0};

    std::atomic<bool> decoding_complete_{false};
    std::atomic<bool>* quit_{nullptr};
};
