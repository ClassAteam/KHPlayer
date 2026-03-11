#include "VideoPlayer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: VideoClient <host> <port>" << std::endl;
        return 1;
    }

    std::string url = "tcp://" + std::string(argv[1]) + ":" + std::string(argv[2]);
    std::cout << "Connecting to " << url << std::endl;

    VideoPlayer player(url);
    // player.test();
    player.test_loop();

    return 0;
}
