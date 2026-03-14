#include "Selector.h"

void Selector::reset(int total, int visible) {
    total_ = total;
    visible_ = visible;
    selected_ = 0;
    scroll_ = 0;
}

void Selector::moveUp() {
    if (selected_ > 0) {
        selected_--;
        if (selected_ < scroll_)
            scroll_ = selected_;
    }
}

void Selector::moveDown() {
    if (selected_ < total_ - 1) {
        selected_++;
        if (selected_ >= scroll_ + visible_)
            scroll_ = selected_ - visible_ + 1;
    }
}
