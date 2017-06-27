/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/io/event/EventGroup.h"

namespace rdd {

EventGroup::EventGroup(size_t capacity)
  : capacity_(capacity) {
  for (size_t i = capacity; i >= 1; --i) {
    groupIds_.push((int)i);
  }
  groupCounts_.resize(capacity + 1, 0);
}

bool EventGroup::createGroup(const std::vector<Event*>& events) {
  WLockGuard guard(lock_);
  if (groupIds_.empty()) {
    if (!doubleSize()) {
      return false;
    }
  }
  for (auto& event : events) {
    if (event->group() != 0) {
      RDDLOG(WARN) << "create group on grouping Event, giveup";
      return false;
    }
  }
  int group = groupIds_.top();
  groupIds_.pop();
  for (auto& event : events) {
    event->setGroup(group);
  }
  groupCounts_[group] = events.size();
  return true;
}

bool EventGroup::finishGroup(Event* event) {
  WLockGuard guard(lock_);
  int group = event->group();
  if (group == 0) {
    return true;
  }
  event->setGroup(0);
  assert(groupCounts_[group] > 0);
  groupCounts_[group]--;
  if (groupCounts_[group] <= 0) {
    groupIds_.push(group);
    return true;
  }
  return false;
}

bool EventGroup::doubleSize() {
  if (capacity_ > 1000000) {
    RDDLOG(ERROR) << "too many groups (>1000000)";
    return false;
  }
  capacity_ *= 2;
  for (size_t i = capacity_; i >= groupCounts_.size(); --i) {
    groupIds_.push((int)i);
  }
  groupCounts_.resize(capacity_ + 1, 0);
  return true;
}

}

