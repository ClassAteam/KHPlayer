#include "VideoServer.h"

#include <algorithm>
#include <arpa/inet.h>
#include <dirent.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

static const std::vector<std::string> SUPPORTED_EXTS = {".mp4", ".mkv", ".avi", ".mov", ".ts"};

static bool has_video_extension(const std::string& name) {
    for (const auto& ext : SUPPORTED_EXTS) {
        if (name.size() >= ext.size() &&
            name.compare(name.size() - ext.size(), ext.size(), ext) == 0)
            return true;
    }
    return false;
}

static std::vector<std::string> scan_directory(const std::string& dir) {
    std::vector<std::string> files;
    DIR* d = opendir(dir.c_str());
    if (!d)
        return files;
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (has_video_extension(name))
            files.push_back(name);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    return files;
}

static void send_all(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return;
        sent += n;
    }
}

static int write_to_socket(void* opaque, const uint8_t* buf, int buf_size) {
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

static void handle_files(int client_fd, const std::vector<std::string>& files) {
    std::string body = "[";
    for (size_t i = 0; i < files.size(); i++) {
        if (i > 0)
            body += ",";
        body += "\"" + files[i] + "\"";
    }
    body += "]";

    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: " +
                           std::to_string(body.size()) + "\r\n\r\n" + body;
    send_all(client_fd, response);
}

static void handle_stream(int client_fd, const std::string& filepath) {
    std::string header = "HTTP/1.0 200 OK\r\nContent-Type: video/mp2t\r\nConnection: close\r\n\r\n";
    send_all(client_fd, header);

    AVFormatContext* in_ctx = nullptr;
    if (avformat_open_input(&in_ctx, filepath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open: " << filepath << std::endl;
        return;
    }
    if (avformat_find_stream_info(in_ctx, nullptr) < 0)
        return;

    AVFormatContext* out_ctx = nullptr;
    avformat_alloc_output_context2(&out_ctx, nullptr, "mpegts", nullptr);

    for (unsigned int i = 0; i < in_ctx->nb_streams; i++) {
        AVStream* out_stream = avformat_new_stream(out_ctx, nullptr);
        avcodec_parameters_copy(out_stream->codecpar, in_ctx->streams[i]->codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    const int avio_buf_size = 65536;
    uint8_t* avio_buf = static_cast<uint8_t*>(av_malloc(avio_buf_size));
    AVIOContext* avio_ctx = avio_alloc_context(avio_buf, avio_buf_size, 1, &client_fd, nullptr,
                                               write_to_socket, nullptr);
    out_ctx->pb = avio_ctx;
    out_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_write_header(out_ctx, nullptr) < 0)
        return;

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
}

static void handle_not_found(int client_fd) {
    send_all(client_fd, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
}

static std::string read_request_line(int client_fd) {
    std::string buf;
    char c;
    while (recv(client_fd, &c, 1, 0) == 1) {
        buf += c;
        if (buf.size() >= 4 && buf.compare(buf.size() - 4, 4, "\r\n\r\n") == 0)
            break;
        if (buf.size() > 8192)
            break;
    }
    size_t eol = buf.find("\r\n");
    return eol != std::string::npos ? buf.substr(0, eol) : "";
}

VideoServer::VideoServer(const std::string& dir, int port) : dir_(dir), port_(port) {}

void VideoServer::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return;
    }
    if (listen(server_fd, 5) < 0) {
        std::cerr << "listen failed" << std::endl;
        return;
    }

    std::cout << "VideoServer listening on port " << port_ << ", serving: " << dir_ << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
            continue;

        std::string req = read_request_line(client_fd);
        size_t s1 = req.find(' ');
        size_t s2 = req.find(' ', s1 + 1);
        if (s1 == std::string::npos || s2 == std::string::npos) {
            close(client_fd);
            continue;
        }
        std::string method = req.substr(0, s1);
        std::string path = req.substr(s1 + 1, s2 - s1 - 1);
        std::cout << method << " " << path << std::endl;

        if (method == "GET" && path == "/files") {
            handle_files(client_fd, scan_directory(dir_));
        } else if (method == "GET" && path.find("/stream?file=") == 0) {
            std::string filename = path.substr(13); // len("/stream?file=") == 13
            if (filename.find('/') != std::string::npos ||
                filename.find("..") != std::string::npos) {
                handle_not_found(client_fd);
            } else {
                handle_stream(client_fd, dir_ + filename);
            }
        } else {
            handle_not_found(client_fd);
        }

        close(client_fd);
    }

    close(server_fd);
}
