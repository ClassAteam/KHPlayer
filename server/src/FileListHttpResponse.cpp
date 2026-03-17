#include "FileListHttpResponse.h"

FileListHttpResponse::FileListHttpResponse(const Directory& directory)
    : files_(directory.getFileNames()) {
    buildJsonList();
    buildResponse();
}

void FileListHttpResponse::buildJsonList() {
    body_ = "[";
    for (size_t i = 0; i < files_.size(); i++) {
        if (i > 0)
            body_ += ",";
        body_ += "\"" + files_[i] + "\"";
    }
    body_ += "]";
}

void FileListHttpResponse::buildResponse() {
    response_ = "HTTP/1.1 200 OK\r\n"
                "Content-Type: " +
                content_type_ +
                "\r\n"
                "Content-Length: " +
                std::to_string(body_.size()) + "\r\n\r\n" + body_;
}

std::string FileListHttpResponse::data() {
    return response_;
}
