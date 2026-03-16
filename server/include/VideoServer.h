#pragma once
#include <string>

class VideoServer {
public:
    VideoServer(const std::string& dir, int port);
    void run();

private:
    std::string dir_;
    int port_;
};
