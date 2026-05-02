#include "ClientConnection.h"
#include "ConnectConfig.h"
#include "Decoder.h"
#include "VideoPlayer.h"
#include "logging.h"
#include <SDL_main.h>
extern "C" {
#include <libavcodec/mediacodec.h>
#include <libavutil/time.h>
}
#include <atomic>
#include <cstdio>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        LOG("integration test starting");
        ConnectConfig config("/sdcard/Android/data/com.simplevideoplayer.integrationtest/files/svp_server.txt");
        LOG("config loaded: %s", config.host().c_str());
        ClientConnection conn(config);
        LOG("connection created");
        auto files = conn.retrieveFiles();
        LOG("receiving file list");
        for (const auto& f : files)
            LOG("  %s", f.c_str());

        std::string file_name("sample1.mp4");
        auto it = std::find(files.begin(), files.end(), file_name);
        if (it == files.end())
            throw std::runtime_error("sample not found on server " + file_name);
        VideoPlayer player(conn.streamUrl(*it));
        player.test_loop();

        std::string file_name2("sample2_h265.mp4");
        auto it2 = std::find(files.begin(), files.end(), file_name2);
        if (it2 == files.end())
            throw std::runtime_error("sample not found on server " + file_name2);
        VideoPlayer player2(conn.streamUrl(*it2), player.getSdlContext());
        player2.test_loop();
        LOG("INTEGRATION TEST PASSED");
    } catch (const std::exception& e) {
        LOG("INTEGRATION TEST FAILED: %s", e.what());
    } catch (...) {
        LOG("INTEGRATION TEST FAILED: unknown exception");
    }
    return 0;
}
