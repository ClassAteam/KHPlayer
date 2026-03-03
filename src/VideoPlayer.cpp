#include "VideoPlayer.h"
#include <chrono>
#include <iostream>

VideoPlayer::VideoPlayer()
    : format_ctx_(nullptr), video_codec_ctx_(nullptr), audio_codec_ctx_(nullptr), sws_ctx_(nullptr),
      swr_ctx_(nullptr), video_stream_index_(-1), audio_stream_index_(-1), window_(nullptr),
      renderer_(nullptr), texture_(nullptr), audio_device_(0), playing_(false), quit_(false),
      paused_(false), current_time_(0.0), video_clock_(0.0), audio_clock_(0.0), frame_timer_(0.0),
      frame_last_pts_(0.0), frame_last_delay_(0.0) {}

VideoPlayer::~VideoPlayer() {
    stop();
    cleanup();
}

bool VideoPlayer::open(const std::string& filename) {
    format_ctx_ = avformat_alloc_context();
    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open video file: " << filename << std::endl;
        return false;
    }

    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        std::cerr << "Failed to find stream information" << std::endl;
        return false;
    }

    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
        AVCodecParameters* codec_params = format_ctx_->streams[i]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);

        if (codec == nullptr) {
            continue;
        }

        if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index_ < 0) {
            video_stream_index_ = i;
            video_codec_ctx_ = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(video_codec_ctx_, codec_params);

            if (avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
                std::cerr << "Failed to open video codec" << std::endl;
                return false;
            }
        } else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0) {
            audio_stream_index_ = i;
            audio_codec_ctx_ = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(audio_codec_ctx_, codec_params);

            if (avcodec_open2(audio_codec_ctx_, codec, nullptr) < 0) {
                std::cerr << "Failed to open audio codec" << std::endl;
                return false;
            }
        }
    }

    if (video_stream_index_ < 0) {
        std::cerr << "No video stream found" << std::endl;
        return false;
    }

    if (!initSDL()) {
        return false;
    }

    std::cout << "Video opened successfully:" << std::endl;
    std::cout << "  Duration: " << getDuration() << " seconds" << std::endl;
    std::cout << "  Resolution: " << video_codec_ctx_->width << "x" << video_codec_ctx_->height
              << std::endl;
    std::cout << "  Has audio: " << (audio_stream_index_ >= 0 ? "Yes" : "No") << std::endl;

    return true;
}

bool VideoPlayer::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window_ = SDL_CreateWindow("Simple Video Player", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, video_codec_ctx_->width,
                               video_codec_ctx_->height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    // Raise window to foreground
    SDL_RaiseWindow(window_);

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 video_codec_ctx_->width, video_codec_ctx_->height);

    if (!texture_) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);

    sws_ctx_ =
        sws_getContext(video_codec_ctx_->width, video_codec_ctx_->height, video_codec_ctx_->pix_fmt,
                       video_codec_ctx_->width, video_codec_ctx_->height, AV_PIX_FMT_BGRA,
                       SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (audio_stream_index_ >= 0) {
        SDL_AudioSpec wanted_spec, obtained_spec;
        wanted_spec.freq = audio_codec_ctx_->sample_rate;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = audio_codec_ctx_->channels;
        wanted_spec.silence = 0;
        wanted_spec.samples = 1024;
        wanted_spec.callback = sdlAudioCallback;
        wanted_spec.userdata = this;

        audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, &obtained_spec, 0);
        if (audio_device_ == 0) {
            std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        } else {
            swr_ctx_ = swr_alloc();
            av_opt_set_int(swr_ctx_, "in_channel_layout", audio_codec_ctx_->channel_layout, 0);
            av_opt_set_int(swr_ctx_, "out_channel_layout", audio_codec_ctx_->channel_layout, 0);
            av_opt_set_int(swr_ctx_, "in_sample_rate", audio_codec_ctx_->sample_rate, 0);
            av_opt_set_int(swr_ctx_, "out_sample_rate", audio_codec_ctx_->sample_rate, 0);
            av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", audio_codec_ctx_->sample_fmt, 0);
            av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
            swr_init(swr_ctx_);
        }
    }

    return true;
}

void VideoPlayer::play() {
    if (playing_) {
        return;
    }

    playing_ = true;
    quit_ = false;
    paused_ = false;

    if (audio_device_ != 0) {
        SDL_PauseAudioDevice(audio_device_, 0);
    }

    decode_thread_ = std::thread(&VideoPlayer::decodeThread, this);
    video_thread_ = std::thread(&VideoPlayer::videoRenderThread, this);
}

void VideoPlayer::pause() {
    paused_ = true;
    if (audio_device_ != 0) {
        SDL_PauseAudioDevice(audio_device_, 1);
    }
}

void VideoPlayer::togglePlayPause() {
    if (paused_) {
        paused_ = false;
        if (audio_device_ != 0) {
            SDL_PauseAudioDevice(audio_device_, 0);
        }
    } else {
        pause();
    }
}

void VideoPlayer::stop() {
    quit_ = true;
    playing_ = false;
    paused_ = false;

    video_cv_.notify_all();
    audio_cv_.notify_all();

    if (decode_thread_.joinable()) {
        decode_thread_.join();
    }
    if (video_thread_.joinable()) {
        video_thread_.join();
    }

    while (!video_queue_.empty()) {
        av_frame_free(&video_queue_.front().frame);
        video_queue_.pop();
    }

    while (!audio_queue_.empty()) {
        av_frame_free(&audio_queue_.front());
        audio_queue_.pop();
    }

    if (audio_device_ != 0) {
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
    }
}

void VideoPlayer::decodeThread() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int64_t video_frame_count = 0;

    while (!quit_ && av_read_frame(format_ctx_, packet) >= 0) {
        if (packet->stream_index == video_stream_index_) {
            avcodec_send_packet(video_codec_ctx_, packet);

            while (avcodec_receive_frame(video_codec_ctx_, frame) == 0) {
                while (video_queue_.size() >= MAX_QUEUE_SIZE && !quit_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                if (quit_)
                    break;

                AVFrame* clone = av_frame_clone(frame);

                // Validate and calculate PTS with fallback
                double pts;
                if (frame->pts != AV_NOPTS_VALUE) {
                    pts = frame->pts * av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
                } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
                    pts = frame->pkt_dts *
                          av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
                    if (video_frame_count < 3) {
                        std::cout << "Warning: Using DTS for frame " << video_frame_count
                                  << std::endl;
                    }
                } else {
                    // Generate PTS from frame count
                    AVRational frame_rate =
                        format_ctx_->streams[video_stream_index_]->avg_frame_rate;
                    pts = video_frame_count * av_q2d(av_inv_q(frame_rate));
                    if (video_frame_count < 3) {
                        std::cout << "Warning: Generating PTS for frame " << video_frame_count
                                  << ": " << pts << std::endl;
                    }
                }

                video_frame_count++;

                std::lock_guard<std::mutex> lock(video_mutex_);
                video_queue_.push({clone, pts});
                video_cv_.notify_one();
            }
        } else if (packet->stream_index == audio_stream_index_) {
            avcodec_send_packet(audio_codec_ctx_, packet);

            while (avcodec_receive_frame(audio_codec_ctx_, frame) == 0) {
                while (audio_queue_.size() >= MAX_QUEUE_SIZE && !quit_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                if (quit_)
                    break;

                AVFrame* clone = av_frame_clone(frame);
                std::lock_guard<std::mutex> lock(audio_mutex_);
                audio_queue_.push(clone);
                audio_cv_.notify_one();
            }
        }

        av_packet_unref(packet);
    }

    // Flush video decoder
    if (video_stream_index_ >= 0 && !quit_) {
        std::cout << "Flushing video decoder..." << std::endl;
        avcodec_send_packet(video_codec_ctx_, nullptr);

        while (avcodec_receive_frame(video_codec_ctx_, frame) == 0 && !quit_) {
            while (video_queue_.size() >= MAX_QUEUE_SIZE && !quit_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (quit_)
                break;

            AVFrame* clone = av_frame_clone(frame);

            // Validate and calculate PTS with fallback
            double pts;
            if (frame->pts != AV_NOPTS_VALUE) {
                pts = frame->pts * av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
            } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
                pts = frame->pkt_dts * av_q2d(format_ctx_->streams[video_stream_index_]->time_base);
            } else {
                AVRational frame_rate = format_ctx_->streams[video_stream_index_]->avg_frame_rate;
                pts = video_frame_count * av_q2d(av_inv_q(frame_rate));
            }

            video_frame_count++;

            std::lock_guard<std::mutex> lock(video_mutex_);
            video_queue_.push({clone, pts});
            video_cv_.notify_one();
        }
        std::cout << "Video decoder flushed. Total frames decoded: " << video_frame_count
                  << std::endl;
    }

    // Flush audio decoder
    if (audio_stream_index_ >= 0 && !quit_) {
        avcodec_send_packet(audio_codec_ctx_, nullptr);

        while (avcodec_receive_frame(audio_codec_ctx_, frame) == 0 && !quit_) {
            while (audio_queue_.size() >= MAX_QUEUE_SIZE && !quit_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (quit_)
                break;

            AVFrame* clone = av_frame_clone(frame);
            std::lock_guard<std::mutex> lock(audio_mutex_);
            audio_queue_.push(clone);
            audio_cv_.notify_one();
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
}

void VideoPlayer::videoRenderThread() {
    frame_timer_ = static_cast<double>(av_gettime()) / 1000000.0;

    while (!quit_) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        std::unique_lock<std::mutex> lock(video_mutex_);
        video_cv_.wait(lock, [this] { return !video_queue_.empty() || quit_; });

        if (quit_)
            break;

        Frame frame_data = video_queue_.front();
        video_queue_.pop();
        lock.unlock();

        AVFrame* bgra_frame = av_frame_alloc();
        bgra_frame->format = AV_PIX_FMT_BGRA;
        bgra_frame->width = video_codec_ctx_->width;
        bgra_frame->height = video_codec_ctx_->height;

        // Allocate frame buffer with proper alignment (32 bytes for SIMD)
        int ret = av_frame_get_buffer(bgra_frame, 32);
        if (ret < 0) {
            std::cerr << "Failed to allocate BGRA frame buffer: " << ret << std::endl;
            av_frame_free(&bgra_frame);
            av_frame_free(&frame_data.frame);
            continue;
        }

        // Convert frame to BGRA format
        int height_ret =
            sws_scale(sws_ctx_, frame_data.frame->data, frame_data.frame->linesize, 0,
                      video_codec_ctx_->height, bgra_frame->data, bgra_frame->linesize);

        if (height_ret != video_codec_ctx_->height) {
            std::cerr << "sws_scale failed: expected " << video_codec_ctx_->height << " but got "
                      << height_ret << std::endl;
            av_frame_free(&bgra_frame);
            av_frame_free(&frame_data.frame);
            continue;
        }

        if (!bgra_frame->data[0]) {
            std::cerr << "Frame data is NULL after conversion" << std::endl;
            av_frame_free(&bgra_frame);
            av_frame_free(&frame_data.frame);
            continue;
        }

        // Initialize timing for first frame
        if (frame_last_pts_ == 0.0 && frame_last_delay_ == 0.0) {
            frame_last_pts_ = frame_data.pts;
            frame_last_delay_ = 1.0 / 25.0; // Default to 25fps
        }

        double delay = frame_data.pts - frame_last_pts_;
        if (delay <= 0 || delay >= 1.0) {
            delay = frame_last_delay_;
        }
        frame_last_delay_ = delay;
        frame_last_pts_ = frame_data.pts;

        double ref_clock = audio_clock_;
        double diff = frame_data.pts - ref_clock;

        double sync_threshold = (delay > 0.01) ? delay : 0.01;
        if (fabs(diff) < 10.0) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }

        frame_timer_ += delay;
        double actual_delay = frame_timer_ - (static_cast<double>(av_gettime()) / 1000000.0);

        if (actual_delay > 0.0 && actual_delay < 1.0) {
            av_usleep(static_cast<unsigned int>(actual_delay * 1000000.0));
        }

        current_time_ = frame_data.pts;

        SDL_UpdateTexture(texture_, nullptr, bgra_frame->data[0], bgra_frame->linesize[0]);

        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);

        av_frame_free(&bgra_frame);
        av_frame_free(&frame_data.frame);
    }
}

void VideoPlayer::sdlAudioCallback(void* userdata, uint8_t* stream, int len) {
    VideoPlayer* player = static_cast<VideoPlayer*>(userdata);
    player->audioCallback(stream, len);
}

void VideoPlayer::audioCallback(uint8_t* stream, int len) {
    SDL_memset(stream, 0, len);

    std::lock_guard<std::mutex> lock(audio_mutex_);

    if (audio_queue_.empty()) {
        return;
    }

    AVFrame* frame = audio_queue_.front();
    audio_queue_.pop();

    int out_buffer_size = av_samples_get_buffer_size(nullptr, audio_codec_ctx_->channels,
                                                     frame->nb_samples, AV_SAMPLE_FMT_S16, 1);

    uint8_t* output = (uint8_t*)av_malloc(out_buffer_size);
    if (!output) {
        av_frame_free(&frame);
        return;
    }

    uint8_t* out_ptr = output;
    int out_samples = swr_convert(swr_ctx_, &out_ptr, frame->nb_samples,
                                  (const uint8_t**)frame->data, frame->nb_samples);

    if (out_samples > 0) {
        int data_size = out_samples * audio_codec_ctx_->channels * sizeof(int16_t);
        int copy_size = (data_size < len) ? data_size : len;
        SDL_MixAudioFormat(stream, output, AUDIO_S16SYS, copy_size, SDL_MIX_MAXVOLUME);

        double pts = frame->pts * av_q2d(format_ctx_->streams[audio_stream_index_]->time_base);
        audio_clock_ = pts;
    }

    av_free(output);
    av_frame_free(&frame);
}

double VideoPlayer::getDuration() const {
    if (format_ctx_) {
        return static_cast<double>(format_ctx_->duration) / AV_TIME_BASE;
    }
    return 0.0;
}

double VideoPlayer::getCurrentTime() const {
    return current_time_;
}

void VideoPlayer::cleanup() {
    cleanupSDL();

    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }

    if (swr_ctx_) {
        swr_free(&swr_ctx_);
    }

    if (video_codec_ctx_) {
        avcodec_free_context(&video_codec_ctx_);
    }

    if (audio_codec_ctx_) {
        avcodec_free_context(&audio_codec_ctx_);
    }

    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
    }
}

void VideoPlayer::cleanupSDL() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void VideoPlayer::seek(double seconds) {
    // Simplified seek - would need more work for production use
    int64_t timestamp = static_cast<int64_t>(
        seconds / av_q2d(format_ctx_->streams[video_stream_index_]->time_base));
    av_seek_frame(format_ctx_, video_stream_index_, timestamp, AVSEEK_FLAG_BACKWARD);
}
