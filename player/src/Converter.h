#pragma once
#include "BoundedQueue.h"
#include "Decoder.h"
#include <atomic>
extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

struct VideoFrame {
    AVFrame* frame;
    double pts;
};

class Converter {
public:
    Converter(Decoder& decoder);
    ~Converter();
    void convert(std::atomic<bool>& quit, std::atomic<bool>& paused);
    std::optional<VideoFrame> getImage();
    void close();

private:
    std::optional<VideoFrame> convertFrame(Frame* frame);
    BoundedQueue<VideoFrame> display_queue_{10};
    SwsContext* scaler_ctx_{nullptr};
    Decoder& decoder_;
    int frame_width_;
    int frame_height_;
};
