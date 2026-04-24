#include "HttpRequest.h"
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <utility>

HttpRequest::HttpRequest(std::string raw_line) : raw_line_(std::move(raw_line)) {
    fprintf(stderr, "request recieved\n");
    firstLine();
    parseMethod();
    parsePath();
    std::cout << method_ << " " << path_ << std::endl;
}

void HttpRequest::firstLine() {

    size_t eol = raw_line_.find("\r\n");
    if (eol == std::string::npos)
        throw std::runtime_error("can't find end of request");
    striped_line_ = raw_line_.substr(0, eol);
}

void HttpRequest::parseMethod() {
    method_end_pos_ = striped_line_.find(' ');
    if (method_end_pos_ == std::string::npos)
        throw std::runtime_error("Can't find method field in http request");
    method_ = striped_line_.substr(0, method_end_pos_);
}

void HttpRequest::parsePath() {
    size_t path_end = striped_line_.find(' ', method_end_pos_ + 1);
    if (path_end == std::string::npos)
        throw std::runtime_error("Can't find path field in http request");

    path_ = striped_line_.substr(method_end_pos_ + 1, path_end - method_end_pos_ - 1);
}

Type HttpRequest::type() {
    if (method_ == "GET" && path_ == "/files") {
        return Type::ListFiles;
    } else if (method_ == "GET" && path_.rfind("/stream?file=", 0) == 0) {
        return Type::FileSelected;
    } else {
        return Type::Unknown;
    }
}

std::string HttpRequest::path() {
    return path_;
}
