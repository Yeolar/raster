/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <queue>

namespace rdd {

template <class T>
class LockedQueue {
public:
  LockedQueue() {}

  void push(const T& value) {
    std::lock_guard<std::mutex> guard(lock_);
    queue_.push(value);
  }
  void push(T&& value) {
    std::lock_guard<std::mutex> guard(lock_);
    queue_.push(std::move(value));
  }

  bool pop(T& value) {
    std::lock_guard<std::mutex> guard(lock_);
    if (queue_.empty()) {
      return false;
    }
    std::swap(value, queue_.front());
    queue_.pop();
    return true;
  }

  T pop() {
    T value = T();
    pop(value);
    return value;
  }

  size_t size() const {
    std::lock_guard<std::mutex> guard(lock_);
    return queue_.size();
  }

private:
  std::queue<T> queue_;
  mutable std::mutex lock_;
};

} // namespace rdd
