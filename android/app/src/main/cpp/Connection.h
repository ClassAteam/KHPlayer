#pragma once
#include "ConnectConfig.h"
#include <string>
#include <vector>

class Connection {
public:
    explicit Connection(ConnectConfig& config);
    std::vector<std::string> retrieveFiles();
    std::string streamUrl(const std::string& filename) const;

private:
    void establishConnection();
    void requestFiles();
    std::string waitResponse();

    int sock_ = -1;
    const std::string host_;
    const int port_;
};
