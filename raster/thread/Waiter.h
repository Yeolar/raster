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
    cond_.wait(lock, [&] { return signal_; });
    signal_ = false;
  }

  template <class Rep, class Period>
  void wait(const std::chrono::duration<Rep, Period>& duration) const {
    std::unique_lock<std::mutex> lock(mtx_);
    cond_.wait_for(lock, duration, [&] { return signal_; });
    signal_ = false;
  }

  void notify_one() const {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      signal_ = true;
    }
    cond_.notify_one();
  }

  void notify_all() const {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      signal_ = true;
    }
    cond_.notify_all();
  }

 private:
  mutable bool signal_{false};
  mutable std::condition_variable cond_;
  mutable std::mutex mtx_;
};

} // namespace rdd
