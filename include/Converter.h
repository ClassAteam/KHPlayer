#include "BoundedQueue.h"
#include "Decoder.h"
#include "SdlContext.h"
#include <atomic>
extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

struct BgraFrame {
    AVFrame* bgra;
    double pts;
};

class Converter {
public:
    Converter(Decoder& decoder, SdlContext& sdl_context);
    void convert(std::atomic<bool>& quit, std::atomic<bool>& paused);
    std::optional<BgraFrame> getImage();
    void close();

private:
    std::optional<BgraFrame> convertFrame(Frame* frame);
    BoundedQueue<BgraFrame> display_queue_{10};
    SwsContext* scaler_ctx_;
    Decoder& decoder_;
    int bgra_frame_count_;
    int frame_width_;
    int frame_height_;
};
