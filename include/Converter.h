#include "Decoder.h"
#include "SdlContext.h"
#include <queue>
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
    void convert();
    std::optional<BgraFrame> getImage();

private:
    std::optional<BgraFrame> convertFrame(Frame* frame);
    std::queue<BgraFrame> display_queue_;
    SwsContext* scaler_ctx_;
    Decoder& decoder_;
    int bgra_frame_count_;
    int frame_width_;
    int frame_height_;
};
