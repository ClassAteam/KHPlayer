#include "ClientConnection.h"
#include "ConnectConfig.h"
#include "FileMenu.h"
#include "VideoPlayer.h"
#include <SDL_main.h>
#include <android/log.h>
#include <cerrno>
#include <cstdio>
#include <string>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)

int main(int argc, char* argv[]) {

    ConnectConfig connect_config;
    ClientConnection connection(connect_config);

    FileMenu menu(connection);
    LOG("main: running FileMenu\n");
    std::string video_url = menu.run();
    LOG("main: FileMenu returned '%s'\n", video_url.c_str());
    if (video_url.empty())
        return 0;

    VideoPlayer player(video_url);
    player.test_loop();

    return 0;
}
