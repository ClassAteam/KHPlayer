#pragma once
#include "Selector.h"
#include "TextRenderer.h"
#include <SDL.h>
#include <string>
#include <vector>

class MenuRenderer {
public:
    MenuRenderer() = default;
    MenuRenderer(SDL_Renderer* renderer, int screen_width, int screen_height);

    void draw(const std::vector<std::string>& files, const Selector& selector);
    void drawError();
    int visibleRows() const;

private:
    void drawBackground();
    void drawItemRows(const std::vector<std::string>& files, const Selector& selector);
    void drawRow(int row_index, bool is_selected, const std::string& filename);
    void drawScrollBar(const std::vector<std::string>& files, const Selector& selector);
    void setColor(SDL_Color c);

    SDL_Renderer* renderer_ = nullptr;
    TextRenderer text_render_;
    int screen_width_ = 0;
    int screen_height_ = 0;

    static constexpr int kHeaderHeight    = 100;
    static constexpr int kItemHeight      = 90;
    static constexpr int kTextScale       = 3;
    static constexpr int kHeaderTextScale = 4;
    static constexpr int kHeaderTextX     = 30;
    static constexpr int kItemTextX       = 24;
    static constexpr int kAccentWidth     = 6;
    static constexpr int kScrollBarWidth  = 8;
    static constexpr int kScrollBarHeight = 60;

    static constexpr SDL_Color kColBg             = {10,  10,  20,  255};
    static constexpr SDL_Color kColHeader         = {20,  60,  120, 255};
    static constexpr SDL_Color kColHeaderText     = {200, 230, 255, 255};
    static constexpr SDL_Color kColRowSelected    = {0,   90,  180, 255};
    static constexpr SDL_Color kColRowNormal      = {35,  35,  50,  255};
    static constexpr SDL_Color kColAccentSelected = {80,  200, 255, 255};
    static constexpr SDL_Color kColAccentNormal   = {60,  80,  120, 255};
    static constexpr SDL_Color kColTextSelected   = {255, 255, 255, 255};
    static constexpr SDL_Color kColTextNormal     = {160, 160, 180, 255};
    static constexpr SDL_Color kColScrollBar      = {80,  120, 200, 255};
    static constexpr SDL_Color kColError          = {120, 20,  20,  255};
};
