#pragma once
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(const std::string& response);
    std::vector<std::string> result();

private:
    void skipHttpHeader();
    void parseJson();

    std::string response_;
    std::string body_;
    std::vector<std::string> result_;
};
