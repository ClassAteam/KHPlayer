#include "VideoServer.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: VideoServer <directory> <port>" << std::endl;
        return 1;
    }
    std::string dir = argv[1];
    if (!dir.empty() && dir.back() != '/')
        dir += '/';
    VideoServer(dir, std::stoi(argv[2])).run();
    return 0;
}
