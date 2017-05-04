/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <set>
#include "rddoc/util/Lock.h"

namespace rdd {

template <class V>
class LockedSet {
public:
  LockedSet() {}

  void insert(const V& v) {
    LockGuard guard(lock_);
    set_.insert(v);
  }

  void erase(const V& v) {
    LockGuard guard(lock_);
    set_.erase(v);
  }

  size_t size() const {
    LockGuard guard(lock_);
    return set_.size();
  }

private:
  std::set<V> set_;
  mutable Lock lock_;
};

}

