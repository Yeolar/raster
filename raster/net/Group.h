/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cassert>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <vector>

namespace raster {

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
  mutable std::mutex lock_;
};

} // namespace raster
