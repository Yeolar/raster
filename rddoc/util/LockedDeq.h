/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <deque>
#include <mutex>

namespace rdd {

template <class V>
class LockedDeq {
public:
  LockedDeq() {}

  void push(const V& v) {
    std::lock_guard<std::mutex> guard(lock_);
    deq_.push_back(v);
  }

  bool pop(V& v) {
    std::lock_guard<std::mutex> guard(lock_);
    if (deq_.empty()) {
      return false;
    }
    v = deq_.front();
    deq_.pop_front();
    return true;
  }

  V pop() {
    V v = V();
    pop(v);
    return v;
  }

  size_t size() const {
    std::lock_guard<std::mutex> guard(lock_);
    return deq_.size();
  }

private:
  std::deque<V> deq_;
  mutable std::mutex lock_;
};

}

