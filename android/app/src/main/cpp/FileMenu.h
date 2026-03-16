#pragma once
#include "Connection.h"
#include "Selector.h"
#include "TextRenderer.h"
#include <SDL.h>
#include <string>
#include <vector>

class FileMenu {
public:
    explicit FileMenu(Connection& connection);
    ~FileMenu();
    std::string run();

private:
    void initWindow();
    void initRenderer();
    void renderErrorScreen();
    void handleEvent(const SDL_Event& e, const std::vector<std::string>& files,
                     std::string& result, bool& running);
    void drawRectangles();
    void drawItemRows(const std::vector<std::string>& files);
    void drawRow(int row_index, bool is_selected, const std::string& filename);
    void drawScrollBar(const std::vector<std::string>& files);

    Connection& connection_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TextRenderer text_render_;
    Selector selector_;
    int screen_width_ = 0;
    int screen_height_ = 0;
    static constexpr int kHeaderHeight = 100;
    static constexpr int kItemHeight = 90;
    static constexpr int kTextScale = 3;
    static constexpr int kHeaderTextScale = 4;
    static constexpr int kHeaderTextX = 30;
    static constexpr int kItemTextX = 24;
    static constexpr int kAccentWidth = 6;
    static constexpr int kScrollBarWidth = 8;
    static constexpr int kScrollBarHeight = 60;

    // Palette
    static constexpr SDL_Color kColBg = {10, 10, 20, 255};
    static constexpr SDL_Color kColHeader = {20, 60, 120, 255};
    static constexpr SDL_Color kColHeaderText = {200, 230, 255, 255};
    static constexpr SDL_Color kColRowSelected = {0, 90, 180, 255};
    static constexpr SDL_Color kColRowNormal = {35, 35, 50, 255};
    static constexpr SDL_Color kColAccentSelected = {80, 200, 255, 255};
    static constexpr SDL_Color kColAccentNormal = {60, 80, 120, 255};
    static constexpr SDL_Color kColTextSelected = {255, 255, 255, 255};
    static constexpr SDL_Color kColTextNormal = {160, 160, 180, 255};
    static constexpr SDL_Color kColScrollBar = {80, 120, 200, 255};
    static constexpr SDL_Color kColError     = {120, 20, 20, 255};

    void setColor(SDL_Color c) {
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    }
};
