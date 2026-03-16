#include "FileMenu.h"
#include <SDL.h>
#include <stdexcept>

#include <algorithm>
#include <string>

#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__)
#endif

FileMenu::FileMenu(Connection& connection) : connection_(connection) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    // Back button must be trapped so it arrives as SDLK_AC_BACK, not SDL_QUIT
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    // Enable joystick events (TV remote D-pad often arrives as a joystick)
    SDL_JoystickEventState(SDL_ENABLE);
}

FileMenu::~FileMenu() = default;

std::string FileMenu::run() {
    auto files = connection_.retrieveFiles();
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
        while (SDL_PollEvent(&e))
            handleEvent(e, files, result, running);

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

void FileMenu::handleEvent(const SDL_Event& e, const std::vector<std::string>& files,
                           std::string& result, bool& running) {
    LOG("FileMenu: event type=0x%x\n", e.type);

    auto nav_select = [&] {
        result = connection_.streamUrl(files[selector_.getCurrentIndex()]);
        running = false;
    };

    if (e.type == SDL_QUIT) {
        running = false;
    } else if (e.type == SDL_KEYDOWN) {
        LOG("FileMenu: key sym=0x%x scancode=%d\n", e.key.keysym.sym, e.key.keysym.scancode);
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
        LOG("FileMenu: joyhat which=%d hat=%d value=%d\n", e.jhat.which, e.jhat.hat, e.jhat.value);
        if (e.jhat.value & SDL_HAT_UP)        selector_.moveUp();
        else if (e.jhat.value & SDL_HAT_DOWN) selector_.moveDown();
    } else if (e.type == SDL_JOYBUTTONDOWN) {
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
    setColor(kColError);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    SDL_Event e;
    while (SDL_WaitEvent(&e)) {
        if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)
            break;
    }
}

void FileMenu::drawRectangles() {
    setColor(kColBg);
    SDL_RenderClear(renderer_);

    setColor(kColHeader);
    SDL_Rect header = {0, 0, screen_width_, kHeaderHeight};
    SDL_RenderFillRect(renderer_, &header);

    int text_y = (kHeaderHeight - kHeaderTextScale * 8) / 2;
    text_render_.drawText(kHeaderTextX, text_y, kHeaderTextScale, kColHeaderText.r,
                          kColHeaderText.g, kColHeaderText.b, "SELECT FILE");
}

void FileMenu::drawItemRows(const std::vector<std::string>& files) {
    int first_visible = selector_.getFirstVisible();
    int selected = selector_.getCurrentIndex();
    int visible = selector_.getVisible();

    int rows_to_render = std::min(visible, (int)files.size() - first_visible);
    for (int i = 0; i < rows_to_render; i++) {
        int file_index = first_visible + i;
        drawRow(i, file_index == selected, files[file_index]);
    }
}

void FileMenu::drawRow(int row_index, bool is_selected, const std::string& filename) {
    int row_y = kHeaderHeight + row_index * kItemHeight;

    SDL_Rect row = {0, row_y, screen_width_, kItemHeight - 2};
    setColor(is_selected ? kColRowSelected : kColRowNormal);
    SDL_RenderFillRect(renderer_, &row);

    SDL_Rect accent = {0, row_y, kAccentWidth, kItemHeight - 2};
    setColor(is_selected ? kColAccentSelected : kColAccentNormal);
    SDL_RenderFillRect(renderer_, &accent);

    SDL_Color tc = is_selected ? kColTextSelected : kColTextNormal;
    int text_y = row_y + (kItemHeight - 2 - kTextScale * 8) / 2;
    text_render_.drawText(kItemTextX, text_y, kTextScale, tc.r, tc.g, tc.b, filename);
}

void FileMenu::drawScrollBar(const std::vector<std::string>& files) {
    int first_visible = selector_.getFirstVisible();
    int visible = selector_.getVisible();

    if ((int)files.size() > visible) {
        float frac = (float)first_visible / (float)(files.size() - visible);
        int travel = screen_height_ - kHeaderHeight - kScrollBarHeight;
        int bar_y = kHeaderHeight + (int)(frac * travel);
        SDL_Rect scrollbar = {screen_width_ - kScrollBarWidth, bar_y, kScrollBarWidth,
                              kScrollBarHeight};
        setColor(kColScrollBar);
        SDL_RenderFillRect(renderer_, &scrollbar);
    }
}
