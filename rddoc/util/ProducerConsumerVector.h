/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <utility>
#include <vector>
#include "rddoc/util/Lock.h"

namespace rdd {

template <class T>
class ProducerConsumerVector {
public:
  ProducerConsumerVector() {
    producer_ = &a_;
    consumer_ = &b_;
  }

  void reserve(size_t n) {
    LockGuard guard(lock_);
    a_.reserve(n);
    b_.reserve(n);
  }

  size_t size() const {
    LockGuard guard(lock_);
    return producer_->size();
  }

  size_t capacity() const {
    LockGuard guard(lock_);
    return producer_->capacity();
  }

  void add(const T& v) {
    LockGuard guard(lock_);
    producer_->push_back(v);
  }

  template <class F>
  void consume(F&& consumer) {
    {
      LockGuard guard(lock_);
      std::swap(producer_, consumer_);
    }
    if (!consumer_->empty()) {
      consumer(*consumer_);
      consumer_->clear();
    }
  }

private:
  std::vector<T> a_, b_;
  std::vector<T>* producer_;
  std::vector<T>* consumer_;
  mutable Lock lock_;
};

}
