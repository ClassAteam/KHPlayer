#include "FileMenu.h"
#include <SDL.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__)
#endif

FileMenu::FileMenu(const std::string& host, int port) : host_(host), port_(port) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    // Back button must be trapped so it arrives as SDLK_AC_BACK, not SDL_QUIT
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    // Enable joystick events (TV remote D-pad often arrives as a joystick)
    SDL_JoystickEventState(SDL_ENABLE);
}

FileMenu::~FileMenu() = default;

std::vector<std::string> FileMenu::fetchFileList() {
    std::vector<std::string> result;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return result;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        return result;
    }
    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return result;
    }

    std::string request =
        "GET /files HTTP/1.0\r\nHost: " + host_ + ":" + std::to_string(port_) + "\r\n\r\n";
    send(sock, request.data(), request.size(), 0);

    std::string response;
    char buf[4096];
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0)
        response.append(buf, static_cast<size_t>(n));
    close(sock);

    // Skip HTTP headers
    size_t body_start = response.find("\r\n\r\n");
    if (body_start == std::string::npos)
        return result;
    const std::string& body = response.substr(body_start + 4);

    // Parse flat JSON array: ["a.mp4","b.mkv",...]
    size_t lb = body.find('[');
    size_t rb = body.find(']');
    if (lb == std::string::npos || rb == std::string::npos || rb <= lb)
        return result;
    std::string inner = body.substr(lb + 1, rb - lb - 1);

    size_t pos = 0;
    while (pos < inner.size()) {
        size_t q1 = inner.find('"', pos);
        if (q1 == std::string::npos)
            break;
        size_t q2 = inner.find('"', q1 + 1);
        if (q2 == std::string::npos)
            break;
        result.push_back(inner.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }
    return result;
}

std::string FileMenu::run() {
    LOG("FileMenu::run() connecting to %s:%d\n", host_.c_str(), port_);
    auto files = fetchFileList();
    LOG("FileMenu::run() got %d files\n", (int)files.size());

    initWindow();
    initRenderer();

    if (files.empty()) {
        renderErrorScreen();
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        return "";
    }

    selector_.reset((int)files.size(), (screen_height_ - kHeaderHeight) / kItemHeight);

    std::string result;
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            // Log every event so we can see exactly what the TV remote sends
            LOG("FileMenu: event type=0x%x\n", e.type);

            auto nav_select = [&] {
                result = "http://" + host_ + ":" + std::to_string(port_) +
                         "/stream?file=" + files[selector_.getCurrentIndex()];
                running = false;
            };

            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                LOG("FileMenu: key sym=0x%x scancode=%d\n", e.key.keysym.sym,
                    e.key.keysym.scancode);
                switch (e.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:        selector_.moveUp();   break;
                case SDLK_DOWN:
                case SDLK_s:        selector_.moveDown(); break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:    nav_select();         break;
                case SDLK_ESCAPE:
                case SDLK_AC_BACK:  running = false;      break;
                default: break;
                }
            } else if (e.type == SDL_JOYHATMOTION) {
                // D-pad on Android TV remote arriving as joystick hat
                LOG("FileMenu: joyhat which=%d hat=%d value=%d\n", e.jhat.which, e.jhat.hat,
                    e.jhat.value);
                if (e.jhat.value & SDL_HAT_UP)        selector_.moveUp();
                else if (e.jhat.value & SDL_HAT_DOWN) selector_.moveDown();
            } else if (e.type == SDL_JOYBUTTONDOWN) {
                // OK/Select button on Android TV remote arriving as joystick button
                LOG("FileMenu: joybutton which=%d button=%d\n", e.jbutton.which, e.jbutton.button);
                nav_select();
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                LOG("FileMenu: controller button=%d\n", e.cbutton.button);
                switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    selector_.moveUp();   break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  selector_.moveDown(); break;
                case SDL_CONTROLLER_BUTTON_A:
                case SDL_CONTROLLER_BUTTON_START:      nav_select();         break;
                case SDL_CONTROLLER_BUTTON_B:
                case SDL_CONTROLLER_BUTTON_BACK:       running = false;      break;
                default: break;
                }
            }
        }

        drawRectangles();
        drawItemRows(files);
        drawScrollBar(files);
        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    return result;
}

void FileMenu::initWindow() {
    window_ = SDL_CreateWindow("Select File", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0,
                               0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window_) {
        LOG("FileMenu: SDL_CreateWindow failed: %s\n", SDL_GetError());
        throw std::runtime_error("SDL_CreateWindow failed");
    }
    SDL_GetWindowSize(window_, &screen_width_, &screen_height_);
    LOG("FileMenu: window created %dx%d\n", screen_width_, screen_height_);
}

void FileMenu::initRenderer() {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        LOG("FileMenu: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        throw std::runtime_error("SDL_CreateRenderer failed");
    }
    text_render_ = TextRenderer(renderer_);
    LOG("FileMenu: renderer created\n");
}

void FileMenu::renderErrorScreen() {
    SDL_SetRenderDrawColor(renderer_, 120, 20, 20, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    SDL_Event e;
    while (SDL_WaitEvent(&e)) {
        if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)
            break;
    }
}

void FileMenu::drawRectangles() {
    // Background
    SDL_SetRenderDrawColor(renderer_, 10, 10, 20, 255);
    SDL_RenderClear(renderer_);

    // Header bar
    SDL_SetRenderDrawColor(renderer_, 20, 60, 120, 255);
    SDL_Rect header = {0, 0, screen_width_, kHeaderHeight};
    SDL_RenderFillRect(renderer_, &header);
    // Header text "SELECT FILE" centered vertically in header, scale=4 → 32px tall
    text_render_.drawText(30, (kHeaderHeight - 32) / 2, 4, 200, 230, 255, "SELECT FILE");
}

void FileMenu::drawItemRows(const std::vector<std::string>& files) {
    int scroll = selector_.getScroll();
    int selected = selector_.getCurrentIndex();
    int visible = selector_.getVisible();

    for (int i = 0; i < visible && (scroll + i) < (int)files.size(); i++) {
        int idx = scroll + i;
        int row_y = kHeaderHeight + i * kItemHeight;

        SDL_Rect row = {0, row_y, screen_width_, kItemHeight - 2};
        if (idx == selected) {
            SDL_SetRenderDrawColor(renderer_, 0, 90, 180, 255);
        } else {
            SDL_SetRenderDrawColor(renderer_, 35, 35, 50, 255);
        }
        SDL_RenderFillRect(renderer_, &row);

        // Left accent bar
        SDL_Rect accent = {0, row_y, 6, kItemHeight - 2};
        if (idx == selected) {
            SDL_SetRenderDrawColor(renderer_, 80, 200, 255, 255);
        } else {
            SDL_SetRenderDrawColor(renderer_, 60, 80, 120, 255);
        }
        SDL_RenderFillRect(renderer_, &accent);

        // Filename text, vertically centred in the row
        int text_y = row_y + (kItemHeight - 2 - kTextScale * 8) / 2;
        if (idx == selected) {
            text_render_.drawText(24, text_y, kTextScale, 255, 255, 255, files[idx]);
        } else {
            text_render_.drawText(24, text_y, kTextScale, 160, 160, 180, files[idx]);
        }
    }
}

void FileMenu::drawScrollBar(const std::vector<std::string>& files) {
    int scroll = selector_.getScroll();
    int visible = selector_.getVisible();

    if ((int)files.size() > visible) {
        float frac = (float)scroll / (float)(files.size() - visible);
        int bar_y = kHeaderHeight + (int)(frac * (screen_height_ - kHeaderHeight - 60));
        SDL_Rect scrollbar = {screen_width_ - 8, bar_y, 8, 60};
        SDL_SetRenderDrawColor(renderer_, 80, 120, 200, 255);
        SDL_RenderFillRect(renderer_, &scrollbar);
    }
}
