#pragma once
#include <string>
#include <vector>

class Directory {
public:
    Directory(std::string path);
    std::vector<std::string> getFileNames() const;

private:
    std::string path_;
};
