#include "MenuRenderer.h"
#include <algorithm>

MenuRenderer::MenuRenderer(SDL_Renderer* renderer, int screen_width, int screen_height)
    : renderer_(renderer),
      text_render_(renderer),
      screen_width_(screen_width),
      screen_height_(screen_height) {}

int MenuRenderer::visibleRows() const {
    return (screen_height_ - kHeaderHeight) / kItemHeight;
}

void MenuRenderer::setColor(SDL_Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
}

void MenuRenderer::draw(const std::vector<std::string>& files, const Selector& selector) {
    drawBackground();
    drawItemRows(files, selector);
    drawScrollBar(files, selector);
}

void MenuRenderer::drawError() {
    setColor(kColError);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
}

void MenuRenderer::drawBackground() {
    setColor(kColBg);
    SDL_RenderClear(renderer_);

    setColor(kColHeader);
    SDL_Rect header = {0, 0, screen_width_, kHeaderHeight};
    SDL_RenderFillRect(renderer_, &header);

    int text_y = (kHeaderHeight - kHeaderTextScale * 8) / 2;
    text_render_.drawText(kHeaderTextX, text_y, kHeaderTextScale,
                          kColHeaderText.r, kColHeaderText.g, kColHeaderText.b, "SELECT FILE");
}

void MenuRenderer::drawItemRows(const std::vector<std::string>& files, const Selector& selector) {
    int first_visible  = selector.getFirstVisible();
    int selected       = selector.getCurrentIndex();
    int rows_to_render = std::min(selector.getVisible(), (int)files.size() - first_visible);

    for (int i = 0; i < rows_to_render; i++) {
        int file_index = first_visible + i;
        drawRow(i, file_index == selected, files[file_index]);
    }
}

void MenuRenderer::drawRow(int row_index, bool is_selected, const std::string& filename) {
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

void MenuRenderer::drawScrollBar(const std::vector<std::string>& files, const Selector& selector) {
    int first_visible = selector.getFirstVisible();
    int visible       = selector.getVisible();

    if ((int)files.size() > visible) {
        float frac   = (float)first_visible / (float)(files.size() - visible);
        int travel   = screen_height_ - kHeaderHeight - kScrollBarHeight;
        int bar_y    = kHeaderHeight + (int)(frac * travel);
        SDL_Rect bar = {screen_width_ - kScrollBarWidth, bar_y, kScrollBarWidth, kScrollBarHeight};
        setColor(kColScrollBar);
        SDL_RenderFillRect(renderer_, &bar);
    }
}
