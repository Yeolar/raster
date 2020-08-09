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

#include <limits>
#include <memory>
#include <queue>
#include <unordered_map>

#include <accelerator/MapUtil.h>

#include "raster/event/Timeout.h"

namespace raster {

template <class T>
class TimedHeap {
 public:
  TimedHeap() {}

  void push(Timeout<T> v) {
    if (!v.data) {
      return;
    }
    map_[v.data] = v;
    heap_.push(v);
  }

  Timeout<T> pop(uint64_t now) {
    while (!heap_.empty()) {
      auto timeout = heap_.top();
      if (acc::get_default(map_, timeout.data) == timeout) {
        if (timeout.deadline <= now) {
          map_.erase(timeout.data);
          heap_.pop();
          return timeout;
        } else {
          break;
        }
      } else {
        heap_.pop();
      }
    }
    return Timeout<T>();
  }

  void erase(T* k) {
    if (!k) {
      return;
    }
    map_.erase(k);
  }

 private:
  std::unordered_map<T*, Timeout<T>> map_;
  std::priority_queue<
    Timeout<T>, std::vector<Timeout<T>>, std::greater<Timeout<T>>> heap_;
};

} // namespace raster
