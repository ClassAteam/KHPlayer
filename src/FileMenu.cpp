#include "FileMenu.h"
#include <SDL.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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

static const int HEADER_H = 100;
static const int ITEM_H = 90;

// ── Embedded 8×8 bitmap font (ASCII 32–127, public domain) ───────────────────
// Each char is 8 bytes, one bit per pixel, MSB = leftmost pixel.
static const uint8_t FONT8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 !
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 #
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 36 $
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // 37 %
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // 38 &
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // 39 '
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // 40 (
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // 41 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 *
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // 44 ,
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // 46 .
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // 47 /
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 48 0
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 49 1
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // 50 2
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // 51 3
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // 52 4
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // 53 5
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // 54 6
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 55 7
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 56 8
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // 57 9
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // 58 :
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // 59 ;
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, // 60 <
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // 61 =
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // 62 >
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, // 63 ?
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, // 64 @
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // 65 A
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // 66 B
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // 67 C
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // 68 D
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, // 69 E
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, // 70 F
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, // 71 G
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // 72 H
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 73 I
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, // 74 J
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, // 75 K
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, // 76 L
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // 77 M
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // 78 N
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 79 O
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, // 80 P
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // 81 Q
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, // 82 R
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, // 83 S
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 84 T
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, // 85 U
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 86 V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 W
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // 88 X
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, // 89 Y
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, // 90 Z
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // 91 [
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 92 backslash
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // 93 ]
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 95 _
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, // 97 a
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, // 98 b
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, // 99 c
    {0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00}, // 100 d
    {0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00}, // 101 e
    {0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00}, // 102 f
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, // 103 g
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, // 104 h
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, // 105 i
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, // 106 j
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, // 107 k
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 108 l
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, // 109 m
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, // 110 n
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // 111 o
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, // 112 p
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, // 113 q
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, // 114 r
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, // 115 s
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, // 116 t
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, // 117 u
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 118 v
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00}, // 119 w
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, // 120 x
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, // 121 y
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, // 122 z
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, // 123 {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 |
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, // 125 }
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 ~
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, // 127 DEL (solid block)
};

// Draw a string at pixel position (x,y) with given scale and colour.
static void drawText(SDL_Renderer* r, int x, int y, int scale,
                     uint8_t cr, uint8_t cg, uint8_t cb,
                     const std::string& text) {
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    int cx = x;
    for (char ch : text) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (uc < 32 || uc > 127) { cx += 8 * scale; continue; }
        const uint8_t* glyph = FONT8[uc - 32];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (1 << col)) {
                    SDL_Rect px = {cx + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(r, &px);
                }
            }
        }
        cx += 8 * scale;
    }
}

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

    SDL_Window* window =
        SDL_CreateWindow("Select File", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
                         SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) {
        LOG("FileMenu: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return "";
    }
    int SCREEN_W = 0, SCREEN_H = 0;
    SDL_GetWindowSize(window, &SCREEN_W, &SCREEN_H);
    LOG("FileMenu: window created %dx%d\n", SCREEN_W, SCREEN_H);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        LOG("FileMenu: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return "";
    }
    LOG("FileMenu: renderer created\n");

    // Show error screen if server unreachable or directory empty, wait for Back/Escape
    if (files.empty()) {
        SDL_SetRenderDrawColor(renderer, 120, 20, 20, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Event e;
        while (SDL_WaitEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)
                break;
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return "";
    }

    int selected = 0;
    int scroll = 0;
    const int visible = (SCREEN_H - HEADER_H) / ITEM_H;
    std::string result;
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            // Log every event so we can see exactly what the TV remote sends
            LOG("FileMenu: event type=0x%x\n", e.type);

            // Helper lambdas for nav actions (captures by reference)
            auto nav_up = [&] {
                if (selected > 0) {
                    selected--;
                    if (selected < scroll)
                        scroll = selected;
                }
            };
            auto nav_down = [&] {
                if (selected < (int)files.size() - 1) {
                    selected++;
                    if (selected >= scroll + visible)
                        scroll = selected - visible + 1;
                }
            };
            auto nav_select = [&] {
                result = "http://" + host_ + ":" + std::to_string(port_) +
                         "/stream?file=" + files[selected];
                running = false;
            };

            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                LOG("FileMenu: key sym=0x%x scancode=%d\n", e.key.keysym.sym,
                    e.key.keysym.scancode);
                switch (e.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:        nav_up();     break;
                case SDLK_DOWN:
                case SDLK_s:        nav_down();   break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:    nav_select(); break;
                case SDLK_ESCAPE:
                case SDLK_AC_BACK:  running = false; break;
                default: break;
                }
            } else if (e.type == SDL_JOYHATMOTION) {
                // D-pad on Android TV remote arriving as joystick hat
                LOG("FileMenu: joyhat which=%d hat=%d value=%d\n", e.jhat.which, e.jhat.hat,
                    e.jhat.value);
                if (e.jhat.value & SDL_HAT_UP)        nav_up();
                else if (e.jhat.value & SDL_HAT_DOWN) nav_down();
            } else if (e.type == SDL_JOYBUTTONDOWN) {
                // OK/Select button on Android TV remote arriving as joystick button
                LOG("FileMenu: joybutton which=%d button=%d\n", e.jbutton.which, e.jbutton.button);
                nav_select();
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                LOG("FileMenu: controller button=%d\n", e.cbutton.button);
                switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    nav_up();     break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  nav_down();   break;
                case SDL_CONTROLLER_BUTTON_A:
                case SDL_CONTROLLER_BUTTON_START:      nav_select(); break;
                case SDL_CONTROLLER_BUTTON_B:
                case SDL_CONTROLLER_BUTTON_BACK:       running = false; break;
                default: break;
                }
            }
        }

        // Background
        SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
        SDL_RenderClear(renderer);

        // Header bar
        SDL_SetRenderDrawColor(renderer, 20, 60, 120, 255);
        SDL_Rect header = {0, 0, SCREEN_W, HEADER_H};
        SDL_RenderFillRect(renderer, &header);
        // Header text "SELECT FILE" centered vertically in header, scale=4 → 32px tall
        drawText(renderer, 30, (HEADER_H - 32) / 2, 4, 200, 230, 255, "SELECT FILE");

        // Item rows
        const int TEXT_SCALE = 3; // 8*3 = 24px tall glyphs
        for (int i = 0; i < visible && (scroll + i) < (int)files.size(); i++) {
            int idx = scroll + i;
            int row_y = HEADER_H + i * ITEM_H;

            SDL_Rect row = {0, row_y, SCREEN_W, ITEM_H - 2};
            if (idx == selected) {
                SDL_SetRenderDrawColor(renderer, 0, 90, 180, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 35, 35, 50, 255);
            }
            SDL_RenderFillRect(renderer, &row);

            // Left accent bar
            SDL_Rect accent = {0, row_y, 6, ITEM_H - 2};
            if (idx == selected) {
                SDL_SetRenderDrawColor(renderer, 80, 200, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 60, 80, 120, 255);
            }
            SDL_RenderFillRect(renderer, &accent);

            // Filename text, vertically centred in the row
            int text_y = row_y + (ITEM_H - 2 - TEXT_SCALE * 8) / 2;
            if (idx == selected) {
                drawText(renderer, 24, text_y, TEXT_SCALE, 255, 255, 255, files[idx]);
            } else {
                drawText(renderer, 24, text_y, TEXT_SCALE, 160, 160, 180, files[idx]);
            }
        }

        // Scrollbar
        if ((int)files.size() > visible) {
            float frac = (float)scroll / (float)(files.size() - visible);
            int bar_y = HEADER_H + (int)(frac * (SCREEN_H - HEADER_H - 60));
            SDL_Rect scrollbar = {SCREEN_W - 8, bar_y, 8, 60};
            SDL_SetRenderDrawColor(renderer, 80, 120, 200, 255);
            SDL_RenderFillRect(renderer, &scrollbar);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return result;
}
