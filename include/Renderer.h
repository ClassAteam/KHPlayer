#include "Decoder.h"
#include "VideoContainer.h"
#include <SdlContext.h>
#include <libswscale/swscale.h>
#include <queue>

extern "C" {
#include <libavutil/frame.h>
}

struct BgraFrame {
    AVFrame* bgra;
    double pts;
};

class Renderer {
public:
    Renderer(Decoder& decoder);
    void renderFrame();
    void recieveImages();

private:
    std::optional<BgraFrame> convertFrame(Frame* frame);
    std::optional<BgraFrame> getImage();
    void delay(double pts);
    SdlContext sdl_context_;
    std::queue<BgraFrame> display_queue_;

    Decoder& decoder_;

    double frame_last_pts_;
    double frame_last_delay_;
    double audio_clock_;
    double frame_timer_;
    double current_time_;
    int bgra_frame_count_;
    int frame_width_;
    int frame_height_;
    SwsContext* scaler_ctx_;
};
