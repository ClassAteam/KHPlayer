#include "Parser.h"
#include <stdexcept>

Parser::Parser(const std::string& response) : response_(response) {
    skipHttpHeader();
    parseJson();
}

void Parser::skipHttpHeader() {
    size_t body_start = response_.find("\r\n\r\n");
    if (body_start == std::string::npos)
        throw std::runtime_error("invalid HTTP response");
    body_ = response_.substr(body_start + 4);
}

void Parser::parseJson() {
    size_t lb = body_.find('[');
    size_t rb = body_.rfind(']');
    if (lb == std::string::npos || rb == std::string::npos || rb <= lb)
        throw std::runtime_error("invalid JSON format");
    std::string inner = body_.substr(lb + 1, rb - lb - 1);

    size_t pos = 0;
    while (pos < inner.size()) {
        size_t q1 = inner.find('"', pos);
        if (q1 == std::string::npos)
            break;
        size_t q2 = inner.find('"', q1 + 1);
        if (q2 == std::string::npos)
            break;
        result_.push_back(inner.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }
}

std::vector<std::string> Parser::result() {
    return result_;
}
