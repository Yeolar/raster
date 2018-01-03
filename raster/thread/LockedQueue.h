/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <queue>

#include "raster/thread/Synchronized.h"

namespace rdd {

template <class T>
class LockedQueue {
 public:
  LockedQueue() {}

  void push(T value) {
    queue_.wlock()->push(std::move(value));
  }

  bool pop(T& value) {
    auto ulockedQueue = queue_.ulock();
    if (!ulockedQueue->empty()) {
      auto wlockedQueue = ulockedQueue.moveFromUpgradeToWrite();
      value = std::move(wlockedQueue->front());
      wlockedQueue->pop();
      return true;
    }
    return false;
  }

  size_t size() const {
    return queue_.rlock()->size();
  }

 private:
  Synchronized<std::queue<T>> queue_;
};

} // namespace rdd
