#pragma once
#include <memory>
#include <string>

class VideoPlayer {
public:
    VideoPlayer(const std::string& filename);
    void test();
    void test_loop();
    ~VideoPlayer();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
