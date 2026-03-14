#pragma once
#include <string>
#include <vector>

// SDL2 file picker: fetches file list from VideoServer via HTTP GET /files,
// renders a navigable list using SDL2 (no SDL_ttf — items shown as colored bars),
// returns "http://host:port/stream?file=name" for the selected file, or "" on cancel.
class FileMenu {
public:
    FileMenu(const std::string& server_host, int port);
    ~FileMenu();
    std::string run();

private:
    std::vector<std::string> fetchFileList();
    std::string host_;
    int port_;
};
