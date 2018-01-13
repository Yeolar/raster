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

#include "raster/parallel/DAG.h"

namespace rdd {

DAG::Key DAG::add(VoidFunc&& func) {
  Key i = nodes_.size();
  nodes_.emplace_back(std::move(func), [&]() { schedule(i); });
  return i;
}

// a -> b
void DAG::dependency(Key a, Key b) {
  nodes_[a].nexts.push_back(b);
  nodes_[b].waitCount++;
  nodes_[b].hasPrev = true;
}

void DAG::schedule(Key i) {
  for (auto key : nodes_[i].nexts) {
    if (--nodes_[key].waitCount == 0) {
      executor_->add([&]() {
        nodes_[key].func();
        nodes_[key].schedule();
      });
    }
  }
}

void DAG::go(VoidFunc&& finishCallback) {
  if (hasCycle()) {
    throw std::runtime_error("Cycle in DAG graph");
  }

  auto sourceKey = add(nullptr);
  auto sinkKey = add(std::move(finishCallback));

  for (Key key = 0; key < nodes_.size() - 2; key++) {
    if (!nodes_[key].hasPrev) {
      dependency(sourceKey, key);
    }
    if (nodes_[key].nexts.empty()) {
      dependency(key, sinkKey);
    }
  }
  schedule(sourceKey);
}

bool DAG::hasCycle() {
  std::vector<std::vector<Key>> nexts;
  for (auto& node : nodes_) {
    nexts.push_back(node.nexts);
  }
  std::vector<size_t> targets(nodes_.size());
  for (auto& edges : nexts) {
    for (auto key : edges) {
      targets[key]++;
    }
  }
  // find starts
  std::vector<Key> keys;
  for (Key key = 0; key < nodes_.size(); key++) {
    if (!nodes_[key].hasPrev) {
      keys.push_back(key);
    }
  }
  // remove edge from start recursively
  while (!keys.empty()) {
    auto key = keys.back();
    keys.pop_back();
    while (!nexts[key].empty()) {
      auto next = nexts[key].back();
      nexts[key].pop_back();
      if (--targets[next] == 0) {
        keys.push_back(next);
      }
    }
  }
  // check if empty
  for (auto& edges : nexts) {
    if (!edges.empty()) {
      return true;
    }
  }
  return false;
}

} // namespace rdd
