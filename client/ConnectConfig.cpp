#include "ConnectConfig.h"
#include <cerrno>
#include <stdexcept>

ConnectConfig::ConnectConfig(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f)
        throw std::runtime_error(std::string("connection config file not found: ") + path);
    parseData(getFileContent(f));
}

ConnectConfig::ConnectConfig() : ConnectConfig(kConfigPath) {}

std::string ConnectConfig::getFileContent(FILE* f) {
    char buf[256] = {};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    return buf;
}

static std::string trimTrailing(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    if (s.empty())
        throw std::runtime_error("config file is empty");
    return s;
}

static size_t findPortSeparatorPos(const std::string& s) {
    size_t colon = s.rfind(':');
    if (colon == std::string::npos)
        throw std::runtime_error("malformed config: expected host:port, got: " + s);
    return colon;
}

void ConnectConfig::parseData(std::string data) {
    data = trimTrailing(data);
    auto port_separator_pos = findPortSeparatorPos(data);
    host_ = data.substr(0, port_separator_pos);
    try {
        port_ = std::stoi(data.substr(port_separator_pos + 1));
    } catch (const std::exception&) {
        throw std::runtime_error("malformed config: invalid port in: " + data);
    }
}
