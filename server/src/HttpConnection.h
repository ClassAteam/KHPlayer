#pragma once
#include <string>

class HttpConnection {
public:
    explicit HttpConnection(int fd);
    ~HttpConnection();
    HttpConnection(const HttpConnection&) = delete;
    HttpConnection& operator=(const HttpConnection&) = delete;
    void sendAll(const std::string& data);

    void sendNotFound() const;
    int rawFd() const {
        return fd_;
    }
    std::string readRawBytes();

private:
    int fd_;
};
