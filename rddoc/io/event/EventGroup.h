/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stack>
#include <vector>
#include "rddoc/io/event/Event.h"
#include "rddoc/util/Lock.h"

namespace rdd {

class EventGroup {
public:
  EventGroup(size_t capacity = 10000);
  ~EventGroup() {}

  bool createGroup(const std::vector<Event*>& events);

  bool finishGroup(Event* event);

  size_t workingGroupCount() const {
    RLockGuard guard(lock_);
    return capacity_ - groupIds_.size();
  }

private:
  bool doubleSize();

  size_t capacity_;
  std::stack<int> groupIds_;     // available group ids
  std::vector<size_t> groupCounts_;
  mutable RWLock lock_;
};

}
