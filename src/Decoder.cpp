#include "Decoder.h"
#include "utility.h"
#include <iostream>
#include <optional>
extern "C" {
#include <libavformat/avformat.h>
}
#include <ostream>
#include <thread>

Decoder::Decoder(const std::string& filename) : container_(filename) {

    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
}

void Decoder::decode() {
    auto fmt_cntxt = container_.getFormatContext();
    int video_stream_index = container_.getVideoStreamIndex();
    int audio_stream_index = container_.getAudioStreamIndex();

    while (av_read_frame(fmt_cntxt, packet_) >= 0) {
        if (packet_->stream_index == video_stream_index) {
            decodeVideoStream();
        } else if (packet_->stream_index == audio_stream_index) {
            decodeAudioStream();
        }

        av_packet_unref(packet_);
    }

    decoding_complete_ = true;
}

void Decoder::decodeVideoStream() {
    auto codec_ctxt = container_.getVideoCodecContext();
    auto time_base = container_.videoTimeBase();
    avcodec_send_packet(codec_ctxt, packet_);

    while (avcodec_receive_frame(codec_ctxt, frame_) == 0) {
        while (video_queue_.size() >= MAX_QUEUE_SIZE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        AVFrame* clone = av_frame_clone(frame_);

        // Validate and calculate PTS with fallback
        double pts;
        if (frame_->pts != AV_NOPTS_VALUE) {
            pts = frame_->pts * time_base;
        } else if (frame_->pkt_dts != AV_NOPTS_VALUE) {
            pts = frame_->pkt_dts * time_base;
            if (video_frame_count_ < 3) {
                std::cout << "Warning: Using DTS for frame " << video_frame_count_ << std::endl;
            }
        } else {
            // Generate PTS from frame count
            AVRational frame_rate = container_.averageFrameRate();
            pts = video_frame_count_ * av_q2d(av_inv_q(frame_rate));
            if (video_frame_count_ < 3) {
                std::cout << "Warning: Generating PTS for frame " << video_frame_count_ << ": "
                          << pts << std::endl;
            }
        }
        video_frame_count_++;

        video_queue_.push({clone, pts});
    }
}

void Decoder::decodeAudioStream() {
    auto codec_ctxt = container_.getAudioCodecContext();

    avcodec_send_packet(codec_ctxt, packet_);
    while (avcodec_receive_frame(codec_ctxt, frame_) == 0) {
        AVFrame* clone = av_frame_clone(frame_);
        audio_queue_.push(clone);
    }
}

void Decoder::inspect_last_frame() {
    auto last_frame = video_queue_.front();
    frame_inspection(15, last_frame.frame);
}

std::optional<Frame> Decoder::getFrame() {
    if (video_queue_.empty())
        return std::nullopt;

    Frame frame = video_queue_.front();
    video_queue_.pop();
    return frame;
}

VideoContainer& Decoder::getContainer() {
    return container_;
}
bool Decoder::isDecodingComplete() {
    return decoding_complete_;
}
bool Decoder::isQueueEmpty() {
    return video_queue_.empty();
}
