#include "FileStream.h"
#include "HttpConnection.h"
#include "VideoReadyHttpResponse.h"

#include <iostream>
#include <sys/socket.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

FileStream::FileStream(HttpConnection& connection, const std::string& file_path)
    : connection_(connection), client_fd_(connection.rawFd()) {
    connection_.sendAll(VideoReadyHttpResponse().data());
    openFile(file_path);
    initOutputContext();
    startStreaming();
}

int FileStream::writeToSocket(void* opaque, const uint8_t* buf, int buf_size) {
    int fd = *static_cast<int*>(opaque);
    ssize_t sent = 0;
    while (sent < buf_size) {
        ssize_t n = ::send(fd, buf + sent, buf_size - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return AVERROR_EOF;
        sent += n;
    }
    return buf_size;
}

void FileStream::openFile(const std::string& file_path) {

    if (avformat_open_input(&in_ctx_, file_path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open: " << file_path << std::endl;
        return;
    }
    if (avformat_find_stream_info(in_ctx_, nullptr) < 0)
        return;
}

void FileStream::initOutputContext() {

    avformat_alloc_output_context2(&out_ctx_, nullptr, "mpegts", nullptr);

    for (unsigned int i = 0; i < in_ctx_->nb_streams; i++) {
        AVStream* out_stream = avformat_new_stream(out_ctx_, nullptr);
        avcodec_parameters_copy(out_stream->codecpar, in_ctx_->streams[i]->codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    static constexpr int kAvioBufSize = 65536;
    uint8_t* avio_buf = static_cast<uint8_t*>(av_malloc(kAvioBufSize));
    avio_ctx_ =
        avio_alloc_context(avio_buf, kAvioBufSize, 1, &client_fd_, nullptr, writeToSocket, nullptr);
    out_ctx_->pb = avio_ctx_;
    out_ctx_->flags |= AVFMT_FLAG_CUSTOM_IO;
}

void FileStream::startStreaming() {
    if (avformat_write_header(out_ctx_, nullptr) < 0)
        return;

    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(in_ctx_, pkt) >= 0) {
        AVStream* in_stream = in_ctx_->streams[pkt->stream_index];
        AVStream* out_stream = out_ctx_->streams[pkt->stream_index];
        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        if (av_interleaved_write_frame(out_ctx_, pkt) < 0)
            break;
        av_packet_unref(pkt);
    }
    av_write_trailer(out_ctx_);

    av_packet_free(&pkt);
    avformat_close_input(&in_ctx_);
    avio_context_free(&avio_ctx_);
    avformat_free_context(out_ctx_);
}
