/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stack>
#include <stdexcept>
#include <vector>
#include <assert.h>

#include "raster/thread/SharedMutex.h"

namespace rdd {

class Group {
 public:
  typedef size_t Key;

  Group(size_t capacity = kMaxCapacity);

  Key create(size_t groupSize);

  bool finish(Key group);

  size_t count() const;

  static constexpr size_t kMaxCapacity = 1 << 14;

 private:
  void increase(size_t bound);

  size_t capacity_;
  std::vector<int> groupCounts_;
  std::stack<Key> groupKeys_; // available group ids
  SharedMutex lock_;
};

} // namespace rdd
