#include "Decoder.h"
#include "libavcodec/avcodec.h"
#include "logging.h"
#include "utility.h"
#include <iostream>
#include <optional>
extern "C" {
#include <inttypes.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
#include <ostream>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

Packet::Packet() {
    // We know that this shouldn't be a case of
    // per-frame allocation, but we are trying
    // to grasp our design.
    packet_ = av_packet_alloc();
    state_ = PacketState::ALLOCATED;
}

Packet::~Packet() {
    av_packet_free(&packet_);
}

void Packet::read(AVFormatContext* ctxt) {
    int result = av_read_frame(ctxt, packet_);
    if (result < 0) {
        state_ = PacketState::BLOCKED;
    }
}

int Packet::stream_index() {
    return packet_->stream_index;
}

void Packet::unref() {
    av_packet_unref(packet_);
}

static void contextToBeginning(AVFormatContext* fmt_cntxt) {
    av_seek_frame(fmt_cntxt, -1, 0, AVSEEK_FLAG_BACKWARD);
}

void Decoder::flushContainerContext() {
    avcodec_flush_buffers(container_.getVideoCodecContext());
    avcodec_flush_buffers(container_.getAudioCodecContext());
}

Decoder::Decoder(const std::string& filename, bool looping, std::atomic<bool>* decoding_control)
    : container_(filename), looping_(looping), decoding_control_(decoding_control) {

    frame_ = av_frame_alloc();
}

// TODO I think decoder should be initialized in two parts.
//  First is responsible for process-wide initialization
//  and the second one is inittialized per-container/per-user.
void Decoder::decode() {
    auto fmt_cntxt = container_.getFormatContext();
    int video_stream_index = container_.getVideoStreamIndex();
    int audio_stream_index = container_.getAudioStreamIndex();

    while (!decoding_control_) {
        auto packet = Packet();
        packet.read(fmt_cntxt);
        if (packet.getState() == PacketState::BLOCKED) {
            if (!looping_)
                break;
            contextToBeginning(fmt_cntxt);
            flushContainerContext();
            video_frame_count_ = 0;
            continue;
        }

        if (packet.stream_index() == video_stream_index) {
            decodeVideoPacket(packet);
        } else if (packet.stream_index() == audio_stream_index) {
            decodeAudioPacket(packet);
        }

        packet.unref();
    }

    decoding_complete_ = true;
    LOG("decoder: EOF, closing queues\n");
    video_queue_.close();
    audio_queue_.close();
}

void Decoder::decodeVideoPacket(Packet& packet) {
    auto codec_ctxt = container_.getVideoCodecContext();
    auto time_base = container_.videoTimeBase();

    packet.send(codec_ctxt);

    int recv_ret;
    while ((recv_ret = avcodec_receive_frame(codec_ctxt, frame_)) == 0) {
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

        AVFrame* output_frame = av_frame_clone(frame_);
        av_frame_unref(frame_);

        if (!video_queue_.push({output_frame, pts})) {
            av_frame_free(&output_frame);
            break;
        }
    }
}

void Decoder::decodeAudioPacket(Packet& packet) {
    auto codec_ctxt = container_.getAudioCodecContext();

    packet.send(codec_ctxt);
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

PacketState Packet::getState() {
    return state_;
}

void Packet::send(AVCodecContext* ctxt) {
    avcodec_send_packet(ctxt, packet_);
    state_ = PacketState::SENT;
}
