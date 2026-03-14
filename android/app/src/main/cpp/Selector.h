#pragma once

class Selector {
public:
    Selector() = default;
    void reset(int total, int visible);
    void moveUp();
    void moveDown();
    int getCurrentIndex() const { return selected_; }
    int getScroll() const { return scroll_; }
    int getVisible() const { return visible_; }

private:
    int selected_ = 0;
    int scroll_ = 0;
    int visible_ = 0;
    int total_ = 0;
};
