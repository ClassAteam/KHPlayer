#pragma once
#include <string>

class ErrorResponse {
public:
    std::string data();

private:
    std::string response_{"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"};
};
