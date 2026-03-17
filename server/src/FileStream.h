#pragma once
#include "HttpConnection.h"
#include <cstdint>
#include <string>
extern "C" {
#include <libavformat/avformat.h>
}

class FileStream {
public:
    FileStream(HttpConnection& connection, const std::string& file_path);

private:
    HttpConnection& connection_;
    int client_fd_;
    static int writeToSocket(void* opaque, const uint8_t* buf, int buf_size);
    void openFile(const std::string& file_path);
    void initOutputContext();
    AVFormatContext* in_ctx_{nullptr};
    AVFormatContext* out_ctx_{nullptr};
    AVIOContext* avio_ctx_{nullptr};
    void startStreaming();
};
