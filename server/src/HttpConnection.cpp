#include "HttpConnection.h"

#include <sys/socket.h>
#include <unistd.h>

HttpConnection::HttpConnection(int fd) : fd_(fd) {}

HttpConnection::~HttpConnection() {
    close(fd_);
}

void HttpConnection::sendAll(const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(fd_, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return;
        sent += n;
    }
}

std::string HttpConnection::readRawBytes() {
    static constexpr size_t kChunkSize = 4096;
    static constexpr size_t kMaxRequestSize = 8192;
    std::string buf;
    char chunk[kChunkSize];
    while (buf.size() < kMaxRequestSize) {
        ssize_t n = recv(fd_, chunk, sizeof(chunk), 0);
        if (n <= 0)
            break;
        buf.append(chunk, n);
        if (buf.find("\r\n\r\n") != std::string::npos)
            break;
    }
    return buf;
}

// void HttpConnection::sendNotFound() const {
//     sendAll(fd_, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
// }
