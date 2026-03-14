#pragma once
#include <SDL.h>
#include <cstdint>
#include <string>

class TextRenderer {
public:
    TextRenderer() = default;
    explicit TextRenderer(SDL_Renderer* renderer);
    void drawText(int x, int y, int scale, uint8_t cr, uint8_t cg, uint8_t cb,
                  const std::string& text);

private:
    SDL_Renderer* renderer_ = nullptr;
};
