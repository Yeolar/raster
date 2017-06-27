/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <queue>
#include "rddoc/util/Lock.h"
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
    LockGuard guard(lock_);
    queue_.push(std::move(item));
    sem_.post();
  }

  T take() {
    while (1) {
      {
        LockGuard guard(lock_);
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
    LockGuard guard(lock_);
    return queue_.size();
  }

private:
  Lock lock_;
  Sem sem_;
  std::queue<T> queue_;
};

}
