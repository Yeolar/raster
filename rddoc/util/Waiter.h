/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include "rddoc/util/noncopyable.h"

namespace rdd {

class Waiter : noncopyable {
public:
  void wait() const {
    std::unique_lock<std::mutex> lock(mtx_);
    while (!signal_) {
      cond_.wait(lock);
    }
    signal_ = false;
  }

  void signal() const {
    std::unique_lock<std::mutex> lock(mtx_);
    signal_ = true;
    cond_.notify_one();
  }

  void broadcast() const {
    std::unique_lock<std::mutex> lock(mtx_);
    signal_ = true;
    cond_.notify_all();
  }

private:
  mutable std::mutex mtx_;
  mutable std::condition_variable cond_;
  mutable bool signal_{false};
};

}

