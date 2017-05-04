/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/EventGroup.h"

namespace rdd {

EventGroup::EventGroup(size_t capacity)
  : capacity_(capacity) {
  for (size_t i = capacity; i >= 1; --i) {
    group_ids_.push((int)i);
  }
  group_counts_.resize(capacity + 1, 0);
}

bool EventGroup::createGroup(const std::vector<Event*>& events) {
  LockGuard guard(lock_);
  if (group_ids_.empty()) {
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
  int group = group_ids_.top();
  group_ids_.pop();
  for (auto& event : events) {
    event->setGroup(group);
  }
  group_counts_[group] = events.size();
  return true;
}

bool EventGroup::finishGroup(Event* event) {
  LockGuard guard(lock_);
  int group = event->group();
  if (group == 0) {
    return true;
  }
  event->setGroup(0);
  assert(group_counts_[group] > 0);
  group_counts_[group]--;
  if (group_counts_[group] <= 0) {
    group_ids_.push(group);
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
  for (size_t i = capacity_; i >= group_counts_.size(); --i) {
    group_ids_.push((int)i);
  }
  group_counts_.resize(capacity_ + 1, 0);
  return true;
}

}

