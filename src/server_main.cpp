#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

static int write_to_socket(void* opaque, uint8_t* buf, int buf_size) {
    int fd = *static_cast<int*>(opaque);
    ssize_t sent = 0;
    while (sent < buf_size) {
        ssize_t n = send(fd, buf + sent, buf_size - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return AVERROR_EOF;
        sent += n;
    }
    return buf_size;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: VideoServer <file> <port>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    int port = std::stoi(argv[2]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(server_fd, 1);

    std::cout << "Listening on port " << port << "..." << std::endl;
    int client_fd = accept(server_fd, nullptr, nullptr);
    std::cout << "Client connected, streaming: " << filename << std::endl;

    // Open input
    AVFormatContext* in_ctx = nullptr;
    if (avformat_open_input(&in_ctx, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open input" << std::endl;
        return 1;
    }
    avformat_find_stream_info(in_ctx, nullptr);

    // Create output context for MPEG-TS
    AVFormatContext* out_ctx = nullptr;
    avformat_alloc_output_context2(&out_ctx, nullptr, "mpegts", nullptr);

    // Copy streams from input to output
    for (unsigned int i = 0; i < in_ctx->nb_streams; i++) {
        AVStream* in_stream = in_ctx->streams[i];
        AVStream* out_stream = avformat_new_stream(out_ctx, nullptr);
        avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    // Wire output to socket via custom AVIOContext
    const int avio_buf_size = 65536;
    uint8_t* avio_buf = static_cast<uint8_t*>(av_malloc(avio_buf_size));
    AVIOContext* avio_ctx =
        avio_alloc_context(avio_buf, avio_buf_size, 1, &client_fd, nullptr, write_to_socket, nullptr);
    out_ctx->pb = avio_ctx;
    out_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    avformat_write_header(out_ctx, nullptr);

    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(in_ctx, pkt) >= 0) {
        AVStream* in_stream = in_ctx->streams[pkt->stream_index];
        AVStream* out_stream = out_ctx->streams[pkt->stream_index];

        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        if (av_interleaved_write_frame(out_ctx, pkt) < 0)
            break;

        av_packet_unref(pkt);
    }

    av_write_trailer(out_ctx);

    av_packet_free(&pkt);
    avformat_close_input(&in_ctx);
    avio_context_free(&avio_ctx);
    avformat_free_context(out_ctx);

    std::cout << "Done." << std::endl;
    close(client_fd);
    close(server_fd);
    return 0;
}
