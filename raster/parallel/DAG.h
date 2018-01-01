/*
 * Copyright (C) 2017, Yeolar
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
