#pragma once
#include "Selector.h"
#include "TextRenderer.h"
#include <SDL.h>
#include <string>
#include <vector>

// SDL2 file picker: fetches file list from VideoServer via HTTP GET /files,
// renders a navigable list using SDL2 (no SDL_ttf — items shown as colored bars),
// returns "http://host:port/stream?file=name" for the selected file, or "" on cancel.
class FileMenu {
public:
    FileMenu(const std::string& host, int port);
    ~FileMenu();
    std::string run();

private:
    std::vector<std::string> fetchFileList();
    void initWindow();
    void initRenderer();
    void renderErrorScreen();
    void drawRectangles();
    void drawItemRows(const std::vector<std::string>& files);
    void drawScrollBar(const std::vector<std::string>& files);

    std::string host_;
    int port_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TextRenderer text_render_;
    Selector selector_;
    int screen_width_ = 0;
    int screen_height_ = 0;
    static constexpr int kHeaderHeight = 100;
    static constexpr int kItemHeight = 90;
    static constexpr int kTextScale = 3;
};
