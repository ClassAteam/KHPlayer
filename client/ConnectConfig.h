#pragma once
#include <cstdio>
#include <string>

class ConnectConfig {
public:
    ConnectConfig();
    explicit ConnectConfig(const char* path);
    const std::string& host() const { return host_; }
    int port() const { return port_; }

private:
    std::string getFileContent(FILE* f);
    void parseData(std::string data);

    static constexpr const char* kConfigPath =
        "/sdcard/Android/data/com.simplevideoplayer/files/svp_server.txt";
    std::string host_;
    int port_{8080};
};
