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

#include <atomic>
#include <stdexcept>
#include <vector>

#include "raster/concurrency/ThreadPoolExecutor.h"

namespace rdd {

class DAG {
 public:
  typedef size_t Key;

  DAG(std::shared_ptr<ThreadPoolExecutor> executor)
    : executor_(executor) {}

  Key add(VoidFunc&& func);

  // a -> b
  void dependency(Key a, Key b);

  void go(VoidFunc&& finishCallback);

  size_t size() const {
    return nodes_.size();
  }

  bool empty() const {
    return nodes_.empty();
  }

 private:
  struct Node {
    explicit Node(VoidFunc&& func_, VoidFunc&& schedule_)
      : func(std::move(func_)),
        schedule(std::move(schedule_)) {}

    Node(Node&& node) {
      std::swap(func, node.func);
      std::swap(schedule, node.schedule);
      std::swap(nexts, node.nexts);
      hasPrev = node.hasPrev;
      waitCount.exchange(node.waitCount.load());
    }

    VoidFunc func;
    VoidFunc schedule;
    std::vector<Key> nexts;
    bool hasPrev{false};
    std::atomic<size_t> waitCount{0};
  };

  bool hasCycle();

  void schedule(Key i);

  std::vector<Node> nodes_;
  std::shared_ptr<ThreadPoolExecutor> executor_;
};

} // namespace rdd
