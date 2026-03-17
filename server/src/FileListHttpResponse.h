#pragma once
#include "Directory.h"
#include <string>
#include <vector>

class FileListHttpResponse {
public:
    FileListHttpResponse(const Directory& directory);
    std::string data();

private:
    void buildJsonList();
    void buildResponse();
    std::string content_type_{"application/json"};
    std::vector<std::string> files_;
    std::string body_;
    std::string json_list_;
    std::string response_;
};
