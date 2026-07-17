#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class Queue {
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
public:
    void push(const T& msg) {
        std::lock_guard lk(mtx_);
        queue_.push(msg);
        cv_.notify_one();
    }

    T pop() {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [this] { return !queue_.empty(); });
        T msg = queue_.front();
        queue_.pop();
        return msg;
    }

    bool tryPop(T& out) {
        std::lock_guard lk(mtx_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop();
        return true;
    }

    bool isEmpty() const {
        std::lock_guard lk(mtx_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard lk(mtx_);
        while (!queue_.empty()) queue_.pop();
    }

    size_t size() const {
        std::lock_guard lk(mtx_);
        return queue_.size();
    }
};
