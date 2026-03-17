#pragma once
#include <string>

class VideoReadyHttpResponse {
public:
    VideoReadyHttpResponse();
    char* data();
    int size();

private:
    std::string response_{
        "HTTP/1.0 200 OK\r\nContent-Type: video/mp2t\r\nConnection: close\r\n\r\n"};
};
