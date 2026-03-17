#pragma once
#include <string>

enum class Type { ListFiles, FileSelected, Unknown };

class HttpRequest {
public:
    HttpRequest(std::string raw_line);
    Type type();
    std::string path();

private:
    std::string raw_line_;
    std::string first_line_;
    std::string striped_line_;
    size_t method_end_pos_;
    void parseMethod();
    void parsePath();
    void firstLine();
    std::string method_;
    std::string path_;
};
