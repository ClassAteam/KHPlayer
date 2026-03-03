#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <string>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <SDL2/SDL.h>

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(const std::string& filename);
    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void seek(double seconds);

    [[nodiscard]] bool isPlaying() const { return playing_; }
    [[nodiscard]] bool isOpen() const { return format_ctx_ != nullptr; }
    [[nodiscard]] double getDuration() const;
    [[nodiscard]] double getCurrentTime() const;

private:
    struct Frame {
        AVFrame* frame;
        double pts;
    };

    void decodeThread();
    void videoRenderThread();
    void audioCallback(uint8_t* stream, int len);

    static void sdlAudioCallback(void* userdata, uint8_t* stream, int len);

    void cleanup();
    bool initSDL();
    void cleanupSDL();

    AVFormatContext* format_ctx_;
    AVCodecContext* video_codec_ctx_;
    AVCodecContext* audio_codec_ctx_;
    SwsContext* sws_ctx_;
    SwrContext* swr_ctx_;

    int video_stream_index_;
    int audio_stream_index_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    SDL_AudioDeviceID audio_device_;

    std::atomic<bool> playing_;
    std::atomic<bool> quit_;
    std::atomic<bool> paused_;
    std::atomic<double> current_time_;

    std::thread decode_thread_;
    std::thread video_thread_;

    std::queue<Frame> video_queue_;
    std::queue<AVFrame*> audio_queue_;
    std::mutex video_mutex_;
    std::mutex audio_mutex_;
    std::condition_variable video_cv_;
    std::condition_variable audio_cv_;

    static const int MAX_QUEUE_SIZE = 25;

    double video_clock_;
    double audio_clock_;
    double frame_timer_;
    double frame_last_pts_;
    double frame_last_delay_;
};

#endif // VIDEO_PLAYER_H
