#include "VideoServer.h"
#include "Directory.h"
#include "ErrorResponse.h"
#include "HttpRequest.h"

#include "FileListHttpResponse.h"
#include "FileStream.h"
#include "HttpConnection.h"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>

static bool isSafeFilename(const std::string& name) {
    return name.find('/') == std::string::npos && name.find("..") == std::string::npos;
}

VideoServer::VideoServer(const std::string& dir, int port) : dir_(dir), port_(port) {}

void VideoServer::run() {
    listen();

    while (true) {
        int connection_fd = accept(socket_fd_, nullptr, nullptr);
        fprintf(stderr, "connection accepted\n");
        if (connection_fd < 0)
            continue;

        HttpConnection conn(connection_fd);
        HttpRequest request(conn.readRawBytes());
        switch (request.type()) {
            case Type::ListFiles:
                handleListFiles(conn);
                break;
            case Type::FileSelected:
                startStream(conn, request.path());
                break;
            case Type::Unknown:
                sendError(conn);
        }
    }
}

void VideoServer::sendError(HttpConnection& conn) {
    ErrorResponse response;
    conn.sendAll(response.data());
}

void VideoServer::handleListFiles(HttpConnection& conn) {
    FileListHttpResponse response{Directory{dir_}};
    conn.sendAll(response.data());
}

static std::string urlDecode(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int c = std::stoi(s.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(c);
            i += 2;
        } else {
            result += s[i];
        }
    }
    return result;
}

void VideoServer::startStream(HttpConnection& conn, const std::string& path) {
    std::string filename = urlDecode(path.substr(13)); // len("/stream?file=") == 13
    if (isSafeFilename(filename))
        FileStream(conn, dir_ + filename);
}

void VideoServer::listen() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed");
    }
    if (::listen(socket_fd_, 5) < 0) {
        throw std::runtime_error("listen failed");
    }
    std::cout << "VideoServer listening on port " << port_ << ", serving: " << dir_ << std::endl;
}
