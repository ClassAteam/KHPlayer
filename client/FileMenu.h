#pragma once
#include "ClientConnection.h"
#include "MenuRenderer.h"
#include "Selector.h"
#include <SDL.h>
#include <string>
#include <vector>

class FileMenu {
public:
    explicit FileMenu(ClientConnection& connection);
    ~FileMenu();
    std::string run();

private:
    void initWindow();
    void initRenderer();
    void renderErrorScreen();
    void handleEvent(const SDL_Event& e, const std::vector<std::string>& files, std::string& result,
                     bool& running);

    ClientConnection& connection_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    MenuRenderer menu_renderer_;
    Selector selector_;
    int screen_width_ = 0;
    int screen_height_ = 0;
};
