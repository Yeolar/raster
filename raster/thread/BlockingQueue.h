/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <queue>

#include "raster/thread/MPMCQueue.h"
#include "raster/thread/Semaphore.h"
#include "raster/thread/Synchronized.h"

namespace rdd {

template <class T>
class BlockingQueue {
 public:
  virtual ~BlockingQueue() {}

  virtual void add(T item) = 0;
  virtual T take() = 0;
  virtual size_t size() = 0;
};

template <class T>
class GenericBlockingQueue : public BlockingQueue<T> {
 public:
  GenericBlockingQueue() {}

  void add(T item) override {
    queue_.wlock()->push(std::move(item));
    sem_.post();
  }

  T take() override {
    while (true) {
      {
        auto ulockedQueue = queue_.ulock();
        if (!ulockedQueue->empty()) {
          auto wlockedQueue = ulockedQueue.moveFromUpgradeToWrite();
          T item = std::move(wlockedQueue->front());
          wlockedQueue->pop();
          return item;
        }
      }
      sem_.wait();
    }
  }

  size_t size() override {
    return queue_.rlock()->size();
  }

 private:
  Semaphore sem_;
  Synchronized<std::queue<T>> queue_;
};

template <class T>
class MPMCBlockingQueue : public BlockingQueue<T> {
 public:
  // Note: The queue pre-allocates all memory for max_capacity
  explicit MPMCBlockingQueue(size_t max_capacity) : queue_(max_capacity) {}

  void add(T item) override {
    if (!queue_.write(std::move(item))) {
      throw std::runtime_error("LifoSemMPMCQueue full, can't add item");
    }
    sem_.post();
  }

  T take() override {
    T item;
    while (!queue_.readIfNotEmpty(item)) {
      sem_.wait();
    }
    return item;
  }

  size_t capacity() {
    return queue_.capacity();
  }

  size_t size() override {
    return queue_.size();
  }

 private:
  Semaphore sem_;
  MPMCQueue<T> queue_;
};

} // namespace rdd
