/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/Group.h"

namespace rdd {

Group::Group(size_t capacity) : capacity_(capacity) {
  increase(1);
}

Group::Key Group::create(size_t groupSize) {
  SharedMutex::WriteHolder guard(lock_);
  if (groupKeys_.empty()) {
    capacity_ *= 2;
    increase(groupCounts_.size());
  }
  assert(groupSize > 0);
  auto group = groupKeys_.top();
  groupKeys_.pop();
  groupCounts_[group] = groupSize;
  return group;
}

bool Group::finish(Group::Key group) {
  if (group == 0) {
    return true;
  }
  SharedMutex::WriteHolder guard(lock_);
  assert(groupCounts_[group] > 0);
  groupCounts_[group]--;
  if (groupCounts_[group] <= 0) {
    groupKeys_.push(group);
    return true;
  }
  return false;
}

size_t Group::count() const {
  SharedMutex::ReadHolder guard(lock_);
  return capacity_ - groupKeys_.size();
}

void Group::increase(size_t bound) {
  for (auto i = capacity_; i >= bound; --i) {
    groupKeys_.push(i);
  }
  groupCounts_.resize(capacity_ + 1, 0);
}

} // namespace rdd
