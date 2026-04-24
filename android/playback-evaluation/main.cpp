#include "ClientConnection.h"
#include "ConnectConfig.h"
#include "Decoder.h"
extern "C" {
#include <libavcodec/mediacodec.h>
#include <libavutil/time.h>
}
#include <atomic>
#include <cstdio>
#include <thread>

static constexpr int MAX_FRAMES = 20;

void evaluateDecoder(const std::string& filename) {
    Decoder decoder(filename);
    decoder.getContainer().openCodecs(nullptr);

    std::atomic<bool> stop_handle{false};
    std::thread decoder_thread([&] { decoder.decode(stop_handle, false); });
    std::thread audio_drain([&] {
        while (decoder.getAudioFrame())
            ;
    });

    int count = 0;
    double prev_circle_end = av_gettime();
    while (auto frame = decoder.getFrame()) {
        double elapsed = (av_gettime() - prev_circle_end);
        int fmt = frame->frame->format;
        count++;
        fprintf(stderr, "frame %d    last circle=%.0fms\n", count, elapsed / 1000.0);

        av_frame_free(&frame->frame);
        prev_circle_end = av_gettime();

        if (count >= MAX_FRAMES) {
            stop_handle = true;
            decoder.closeQueues();
            break;
        }
    }

    fprintf(stderr, "done: %d frames\n", count);
    decoder_thread.join();
    audio_drain.join();
}

int main() {
    ConnectConfig config;
    ClientConnection conn(config);
    auto files = conn.retrieveFiles();
    fprintf(stderr, "recieving file list\n");
    for (const auto& f : files)
        fprintf(stderr, "%s\n", f.c_str());

    std::string file_name("sample2.mkv");
    auto it = std::find(files.begin(), files.end(), file_name);
    if (it == files.end())
        throw std::runtime_error("sample not found on server " + file_name);
    evaluateDecoder(conn.streamUrl(*it)); // evaluateDecoder(conn.streamUrl("sample2.mkv"));
}
