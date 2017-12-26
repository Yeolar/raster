/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <condition_variable>
#include <mutex>

namespace rdd {

class Waiter {
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
  mutable bool signal_{false};
  mutable std::condition_variable cond_;
  mutable std::mutex mtx_;
};

} // namespace rdd