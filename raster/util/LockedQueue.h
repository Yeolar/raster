/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <queue>

#include "raster/util/RWLock.h"

namespace rdd {

template <class T>
class LockedQueue {
public:
  LockedQueue() {}

  void push(const T& value) {
    WLockGuard guard(lock_);
    queue_.push(value);
  }
  void push(T&& value) {
    WLockGuard guard(lock_);
    queue_.push(std::move(value));
  }

  bool pop(T& value) {
    WLockGuard guard(lock_);
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
    RLockGuard guard(lock_);
    return queue_.size();
  }

private:
  std::queue<T> queue_;
  mutable RWLock lock_;
};

} // namespace rdd
