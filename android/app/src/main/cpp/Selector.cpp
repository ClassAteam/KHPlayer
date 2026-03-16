#include "Selector.h"

void Selector::reset(int total, int visible) {
    total_ = total;
    visible_ = visible;
    selected_ = 0;
    first_visible_ = 0;
}

void Selector::moveUp() {
    if (selected_ > 0) {
        selected_--;
        if (selected_ < first_visible_)
            first_visible_ = selected_;
    }
}

void Selector::moveDown() {
    if (selected_ < total_ - 1) {
        selected_++;
        if (selected_ >= first_visible_ + visible_)
            first_visible_ = selected_ - visible_ + 1;
    }
}
