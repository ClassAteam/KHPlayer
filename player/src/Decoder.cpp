#include "Decoder.h"
#include "utility.h"
#include <iostream>
#include <optional>
extern "C" {
#include <libavformat/avformat.h>
}
#include <ostream>
#include <thread>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

Decoder::Decoder(const std::string& filename) : container_(filename) {

    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
}

void Decoder::decode(std::atomic<bool>& quit, bool loop) {
    quit_ = &quit;
    auto fmt_cntxt = container_.getFormatContext();
    int video_stream_index = container_.getVideoStreamIndex();
    int audio_stream_index = container_.getAudioStreamIndex();

    while (!quit) {

        if (av_read_frame(fmt_cntxt, packet_) < 0) {
            if (!loop)
                break;
            av_seek_frame(fmt_cntxt, -1, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(container_.getVideoCodecContext());
            avcodec_flush_buffers(container_.getAudioCodecContext());
            video_frame_count_ = 0;
            continue;
        }

        if (packet_->stream_index == video_stream_index) {
            decodeVideoPacket();
        } else if (packet_->stream_index == audio_stream_index) {
            decodeAudioPacket();
        }

        av_packet_unref(packet_);
    }

    decoding_complete_ = true;
    video_queue_.close();
    audio_queue_.close();
}

void Decoder::decodeVideoPacket() {
    auto codec_ctxt = container_.getVideoCodecContext();
    auto time_base = container_.videoTimeBase();
    avcodec_send_packet(codec_ctxt, packet_);

    while (avcodec_receive_frame(codec_ctxt, frame_) == 0) {

        double pts;
        if (frame_->pts != AV_NOPTS_VALUE) {
            pts = frame_->pts * time_base;
        } else if (frame_->pkt_dts != AV_NOPTS_VALUE) {
            pts = frame_->pkt_dts * time_base;
            if (video_frame_count_ < 3) {
                std::cout << "Warning: Using DTS for frame " << video_frame_count_ << std::endl;
            }
        } else {
            AVRational frame_rate = container_.averageFrameRate();
            pts = video_frame_count_ * av_q2d(av_inv_q(frame_rate));
            if (video_frame_count_ < 3) {
                std::cout << "Warning: Generating PTS for frame " << video_frame_count_ << ": "
                          << pts << std::endl;
            }
        }
        video_frame_count_++;

        AVFrame* clone = av_frame_clone(frame_);
        if (!video_queue_.push({clone, pts})) {
            av_frame_free(&clone);
            break;
        }
    }
}

void Decoder::decodeAudioPacket() {
    auto codec_ctxt = container_.getAudioCodecContext();

    avcodec_send_packet(codec_ctxt, packet_);
    while (avcodec_receive_frame(codec_ctxt, frame_) == 0) {
        AVFrame* clone = av_frame_clone(frame_);
        if (!audio_queue_.push(clone)) {
            av_frame_free(&clone);
            break;
        }
    }
}

void Decoder::inspect_last_frame() {
    auto frame = video_queue_.try_pop();
    if (frame) {
        frame_inspection(15, frame->frame);
        av_frame_free(&frame->frame);
    }
}

std::optional<Frame> Decoder::getFrame() {
    return video_queue_.pop();
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

std::optional<AVFrame*> Decoder::getAudioFrame() {
    return audio_queue_.pop();
}

bool Decoder::isAudioQueueEmpty() {
    return audio_queue_.empty();
}

void Decoder::closeQueues() {
    video_queue_.close();
    audio_queue_.close();
}
