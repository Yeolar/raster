/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/net/Group.h"

namespace rdd {

using acc::SharedMutex;

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
