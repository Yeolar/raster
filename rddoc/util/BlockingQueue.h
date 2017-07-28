/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <queue>
#include "rddoc/util/Sem.h"

namespace rdd {

template <class T>
class BlockingQueue {
public:
  virtual void add(T item) = 0;
  virtual T take() = 0;
  virtual size_t size() = 0;
};

template <class T>
class GenericBlockingQueue : public BlockingQueue<T> {
public:
  void add(T item) {
    std::lock_guard<std::mutex> guard(lock_);
    queue_.push(std::move(item));
    sem_.post();
  }

  T take() {
    while (1) {
      {
        std::lock_guard<std::mutex> guard(lock_);
        if (queue_.size() > 0) {
          auto item = std::move(queue_.front());
          queue_.pop();
          return item;
        }
      }
      sem_.wait();
    }
  }

  size_t size() {
    std::lock_guard<std::mutex> guard(lock_);
    return queue_.size();
  }

private:
  std::mutex lock_;
  Sem sem_;
  std::queue<T> queue_;
};

} // namespace rdd
