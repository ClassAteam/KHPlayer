#include "VideoPlayer.h"

int main(int argc, char* argv[]) {

    std::string video_file = argv[1];
    VideoPlayer player(video_file);
    player.test();

    return 0;
}
