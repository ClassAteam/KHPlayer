#pragma once

class Selector {
public:
    Selector() = default;
    void reset(int total, int visible);
    void moveUp();
    void moveDown();
    int getCurrentIndex() const { return selected_; }
    int getFirstVisible() const { return first_visible_; }
    int getVisible() const { return visible_; }

private:
    int selected_ = 0;
    int first_visible_ = 0;
    int visible_ = 0;
    int total_ = 0;
};
