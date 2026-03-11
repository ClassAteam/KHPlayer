#include "VideoPlayer.h"
#ifdef ANDROID
#include <SDL_main.h>
#endif

int main(int argc, char* argv[]) {
#ifdef ANDROID
    // On Android there are no command-line args.
    // Push a test file to the device first:
    //   adb push fixtures/sample.mp4 /sdcard/sample.mp4
    std::string video_file = "/sdcard/sample.mp4";
#else
    std::string video_file = argv[1];
#endif
    VideoPlayer player(video_file);
    player.test_loop();

    return 0;
}
