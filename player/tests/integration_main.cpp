#include "VideoPlayer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: SimpleVideoPlayer <file_or_url>" << std::endl;
        return 1;
    }
    VideoPlayer player(argv[1]);
    player.test_loop();
    return 0;
}
