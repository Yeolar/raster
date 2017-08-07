/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stack>
#include <stdexcept>
#include <vector>
#include <assert.h>
#include "raster/util/RWLock.h"

namespace rdd {

class Group {
public:
  typedef size_t Key;

  Group(size_t capacity = 100)
    : capacity_(capacity) {
    increase(1);
  }

  Key create(size_t groupSize) {
    WLockGuard guard(lock_);
    if (groupKeys_.empty()) {
      capacity_ *= 2;
      increase(groupCounts_.size());
    }
    assert(groupSize > 0);
    Key group = groupKeys_.top();
    groupKeys_.pop();
    groupCounts_[group] = groupSize;
    return group;
  }

  bool finish(Key group) {
    WLockGuard guard(lock_);
    if (group == 0) {
      return true;
    }
    assert(groupCounts_[group] > 0);
    groupCounts_[group]--;
    if (groupCounts_[group] <= 0) {
      groupKeys_.push(group);
      return true;
    }
    return false;
  }

  size_t count() const {
    RLockGuard guard(lock_);
    return capacity_ - groupKeys_.size();
  }

private:
  void increase(size_t bound) {
    for (Key i = capacity_; i >= bound; --i) {
      groupKeys_.push(i);
    }
    groupCounts_.resize(capacity_ + 1, 0);
  }

  size_t capacity_;
  std::stack<Key> groupKeys_;     // available group ids
  std::vector<size_t> groupCounts_;
  mutable RWLock lock_;
};

} // namespace rdd
