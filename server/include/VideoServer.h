#pragma once
#include "HttpConnection.h"
#include <string>

class VideoServer {
public:
    VideoServer(const std::string& dir, int port);
    void run();

private:
    void sendError(HttpConnection& conn);
    void startStream(HttpConnection& conn, const std::string& path);
    void handleListFiles(HttpConnection& conn);
    void listen();
    std::string dir_;
    int port_;
    int socket_fd_;
};
