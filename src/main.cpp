#include "VideoPlayer.h"
#include <iostream>
#include <iomanip>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <video_file>" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  SPACE    - Play/Pause" << std::endl;
    std::cout << "  Q/ESC    - Quit" << std::endl;
    std::cout << "  LEFT     - Seek backward 10 seconds" << std::endl;
    std::cout << "  RIGHT    - Seek forward 10 seconds" << std::endl;
}

void printStatus(const VideoPlayer& player) {
    double current = player.getCurrentTime();
    double duration = player.getDuration();
    int progress = static_cast<int>((current / duration) * 50);

    std::cout << "\r[";
    for (int i = 0; i < 50; i++) {
        if (i < progress) {
            std::cout << "=";
        } else if (i == progress) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] ";

    int current_min = static_cast<int>(current) / 60;
    int current_sec = static_cast<int>(current) % 60;
    int duration_min = static_cast<int>(duration) / 60;
    int duration_sec = static_cast<int>(duration) % 60;

    std::cout << std::setfill('0') << std::setw(2) << current_min << ":"
              << std::setfill('0') << std::setw(2) << current_sec << " / "
              << std::setfill('0') << std::setw(2) << duration_min << ":"
              << std::setfill('0') << std::setw(2) << duration_sec;

    std::cout << " " << (player.isPlaying() ? "[Playing]" : "[Paused] ");
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string video_file = argv[1];
    VideoPlayer player;

    std::cout << "Opening video file: " << video_file << std::endl;

    if (!player.open(video_file)) {
        std::cerr << "Failed to open video file" << std::endl;
        return 1;
    }

    std::cout << "\nStarting playback..." << std::endl;
    std::cout << "\nControls: SPACE=Play/Pause, LEFT/RIGHT=Seek, Q/ESC=Quit\n" << std::endl;

    player.play();

    SDL_Event event;
    bool running = true;
    auto last_status_update = std::chrono::steady_clock::now();

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_SPACE:
                            player.togglePlayPause();
                            break;

                        case SDLK_q:
                        case SDLK_ESCAPE:
                            running = false;
                            break;

                        case SDLK_LEFT:
                            player.seek(player.getCurrentTime() - 10.0);
                            break;

                        case SDLK_RIGHT:
                            player.seek(player.getCurrentTime() + 10.0);
                            break;
                    }
                    break;
            }
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_status_update);

        if (elapsed.count() >= 100) {
            printStatus(player);
            last_status_update = now;
        }

        if (player.getCurrentTime() >= player.getDuration() - 0.1) {
            std::cout << "\n\nPlayback finished!" << std::endl;
            running = false;
        }

        SDL_Delay(10);
    }

    std::cout << "\n\nStopping playback..." << std::endl;
    player.stop();

    return 0;
}
