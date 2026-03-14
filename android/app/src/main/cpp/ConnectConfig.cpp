#include "ConnectConfig.h"
#include <android/log.h>
#include <cerrno>
#include <stdexcept>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)

ConnectConfig::ConnectConfig() {
    LOG("ConnectConfig: opening %s\n", kConfigPath);
    FILE* f = fopen(kConfigPath, "r");
    if (!f) {
        LOG("ConnectConfig: fopen failed, errno=%d\n", errno);
        throw std::runtime_error("connection config file not found");
    }
    auto content = getFileContent(f);
    parseData(content);
}

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
