/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stack>
#include <vector>
#include "rddoc/net/Event.h"
#include "rddoc/util/Lock.h"

namespace rdd {

class EventGroup {
public:
  EventGroup(size_t capacity = 10000);
  ~EventGroup() {}

  bool createGroup(const std::vector<Event*>& events);

  bool finishGroup(Event* event);

  size_t workingGroupCount() const {
    LockGuard guard(lock_);
    return capacity_ - group_ids_.size();
  }

private:
  bool doubleSize();

  size_t capacity_;
  std::stack<int> group_ids_;     // available group ids
  std::vector<size_t> group_counts_;
  mutable Lock lock_;
};

}
