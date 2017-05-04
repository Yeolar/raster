/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdint.h>
#include "rddoc/util/RWSpinLock.h"

namespace rdd {

class Counter {
public:
  Counter() {}
  virtual ~Counter() {}

  int64_t get() const {
    RWSpinLock::ReadHolder guard(lock_);
    return value_;
  }

  int64_t set(int64_t value) {
    RWSpinLock::WriteHolder guard(lock_);
    value_ = value;
    return value_;
  }

  int64_t incr(int64_t n = 1) {
    RWSpinLock::WriteHolder guard(lock_);
    value_ += n;
    return value_;
  }

  int64_t decr(int64_t n = 1) {
    RWSpinLock::WriteHolder guard(lock_);
    value_ -= n;
    return value_;
  }

private:
  int64_t value_{0};
  mutable RWSpinLock lock_;
};

}

