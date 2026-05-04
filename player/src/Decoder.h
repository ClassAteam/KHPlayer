#pragma once
#include "BoundedQueue.h"
#include "VideoContainer.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <atomic>
#include <optional>

enum class PacketState { ALLOCATED, POPULATED, VERIFIED, BLOCKED, SENT };

class Packet {
public:
    Packet();
    ~Packet();
    void read(AVFormatContext* ctxt);
    int stream_index();
    PacketState getState();
    void unref();
    void send(AVCodecContext* ctxt);

private:
    PacketState state_;
    AVPacket* packet_;
};

struct Frame {
    AVFrame* frame;
    double pts;
};
class Decoder {
public:
    Decoder(const std::string& filename, bool looping = false,
            std::atomic<bool>* decoding_control = nullptr);
    void decode();
    void inspect_last_frame();
    std::optional<Frame> getFrame();
    std::optional<AVFrame*> getAudioFrame();
    VideoContainer& getContainer();
    bool isDecodingComplete();
    bool isQueueEmpty();
    bool isAudioQueueEmpty();
    void closeQueues();

private:
    void flushContainerContext();
    void decodeVideoPacket(Packet& packet);
    void decodeAudioPacket(Packet& packet);
    VideoContainer container_;

    static const int MAX_QUEUE_SIZE = 25;
    BoundedQueue<Frame> video_queue_{MAX_QUEUE_SIZE};
    BoundedQueue<AVFrame*> audio_queue_{MAX_QUEUE_SIZE};
    AVFrame* frame_;
    int64_t video_frame_count_{0};

    std::atomic<bool> decoding_complete_{false};
    bool looping_{false};
    std::atomic<bool>* decoding_control_{nullptr};
};
