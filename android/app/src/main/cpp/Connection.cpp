#include "Connection.h"
#include "Parser.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__)
#endif

Connection::Connection(ConnectConfig& config) : host_(config.host()), port_(config.port()) {
    establishConnection();
}

void Connection::establishConnection() {
    LOG("Connection: connecting to %s:%d\n", host_.c_str(), port_);
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0)
        throw std::runtime_error("failed to obtain the socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        close(sock_);
        throw std::runtime_error("invalid IP address");
    }
    if (connect(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock_);
        throw std::runtime_error("failed to connect to the socket");
    }
}

void Connection::requestFiles() {
    std::string request =
        "GET /files HTTP/1.0\r\nHost: " + host_ + ":" + std::to_string(port_) + "\r\n\r\n";
    send(sock_, request.data(), request.size(), 0);
}

std::string Connection::waitResponse() {
    std::string response;
    char buf[4096];
    ssize_t n;
    while ((n = recv(sock_, buf, sizeof(buf), 0)) > 0)
        response.append(buf, static_cast<size_t>(n));
    close(sock_);
    return response;
}

static std::string urlEncode(const std::string& s) {
    std::string result;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            result += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            result += buf;
        }
    }
    return result;
}

std::string Connection::streamUrl(const std::string& filename) const {
    return "http://" + host_ + ":" + std::to_string(port_) + "/stream?file=" + urlEncode(filename);
}

std::vector<std::string> Connection::retrieveFiles() {
    requestFiles();
    auto response = waitResponse();
    Parser parser(response);
    return parser.result();
}
