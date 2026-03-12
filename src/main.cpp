#include "VideoPlayer.h"
#ifdef ANDROID
#include "FileMenu.h"
#include <SDL_main.h>
#include <android/log.h>
#include <cerrno>
#include <cstdio>
#include <string>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#endif

int main(int argc, char* argv[]) {
#ifdef ANDROID
    // Read server address from /sdcard/svp_server.txt (format: "192.168.x.x:8080")
    std::string host;
    int port = 8080;
    {
        static const char* CONFIG = "/sdcard/Android/data/com.simplevideoplayer/files/svp_server.txt";
        LOG("main: opening %s\n", CONFIG);
        FILE* f = fopen(CONFIG, "r");
        if (!f) {
            LOG("main: fopen failed, errno=%d\n", errno);
            return 1;
        }
        char buf[256] = {};
        if (fgets(buf, sizeof(buf), f))
            ; // result used below
        fclose(f);
        std::string line = buf;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        size_t colon = line.rfind(':');
        if (colon != std::string::npos) {
            host = line.substr(0, colon);
            port = std::stoi(line.substr(colon + 1));
        } else {
            host = line;
        }
    }

    LOG("main: server=%s port=%d\n", host.c_str(), port);
    FileMenu menu(host, port);
    LOG("main: running FileMenu\n");
    std::string video_url = menu.run();
    LOG("main: FileMenu returned '%s'\n", video_url.c_str());
    if (video_url.empty())
        return 0;

    VideoPlayer player(video_url);
    player.test_loop();
#else
    if (argc < 2) {
        return 1;
    }
    VideoPlayer player(argv[1]);
    player.test_loop();
#endif

    return 0;
}
