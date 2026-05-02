#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t max_size) : max_size_(max_size) {}

    // Blocks when full. Returns false if closed (item discarded).
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] { return queue_.size() < max_size_ || closed_; });
        if (closed_) return false;
        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Blocks when empty. Returns nullopt when closed and empty.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    // Non-blocking. Returns nullopt if empty (for Renderer and SDL callback).
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    // Wake all blocked push/pop calls. Subsequent push() returns false, pop() drains then returns nullopt.
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        not_full_.notify_all();
        not_empty_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    bool isClosed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    size_t max_size_;
    bool closed_{false};
};
