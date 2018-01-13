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

#pragma once

#include <queue>

#include "raster/thread/Synchronized.h"

namespace rdd {

template <class T>
class LockedQueue {
 public:
  LockedQueue() {}

  void push(T value) {
    queue_.wlock()->push(std::move(value));
  }

  bool pop(T& value) {
    auto ulockedQueue = queue_.ulock();
    if (!ulockedQueue->empty()) {
      auto wlockedQueue = ulockedQueue.moveFromUpgradeToWrite();
      value = std::move(wlockedQueue->front());
      wlockedQueue->pop();
      return true;
    }
    return false;
  }

  size_t size() const {
    return queue_.rlock()->size();
  }

 private:
  Synchronized<std::queue<T>> queue_;
};

} // namespace rdd
