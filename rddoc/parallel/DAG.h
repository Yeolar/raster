/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <vector>
#include "rddoc/coroutine/Executor.h"
#include "rddoc/util/Logging.h"

namespace rdd {

class DAG {
public:
  typedef size_t Key;

  Key add(const ExecutorPtr& executor) {
    executor->addSchedule(std::bind(&DAG::schedule, this, nodes_.size()));
    return addNode(executor);
  }

  // a -> b
  void dependency(Key a, Key b) {
    nodes_[a].nexts.push_back(b);
    nodes_[b].waitCount++;
    nodes_[b].hasPrev = true;
  }

  size_t size() const { return nodes_.size(); }
  bool empty() const { return nodes_.empty(); }

  virtual void execute(const ExecutorPtr& executor) {
    executor->run();
    executor->callback();
    executor->schedule();
  }

  void schedule(Key i) {
    for (auto key : nodes_[i].nexts) {
      if (--nodes_[key].waitCount == 0) {
        execute(nodes_[key].executor);
      }
    }
  }

  Key go(const ExecutorPtr& executor) {
    if (hasCycle()) {
      throw std::runtime_error("Cycle in DAG graph");
    }
    std::vector<Key> rootNodes;
    std::vector<Key> leafNodes;
    for (Key key = 0; key < nodes_.size(); key++) {
      if (!nodes_[key].hasPrev) {
        rootNodes.push_back(key);
      }
      if (nodes_[key].nexts.empty()) {
        leafNodes.push_back(key);
      }
    }
    auto sourceKey = addNode(nullptr);
    for (auto key : rootNodes) {
      dependency(sourceKey, key);
    }
    auto sinkKey = addNode(executor);
    for (auto key : leafNodes) {
      dependency(key, sinkKey);
    }
    return sourceKey;
  }

private:
  struct Node {
    Node(const ExecutorPtr& executor_) : executor(executor_) {}

    Node(Node&& node) {
      std::swap(executor, node.executor);
      std::swap(nexts, node.nexts);
      std::swap(hasPrev, node.hasPrev);
      waitCount.exchange(node.waitCount.load());
    }

    ExecutorPtr executor;
    std::vector<Key> nexts;
    bool hasPrev{false};
    std::atomic<size_t> waitCount{0};
  };

  Key addNode(const ExecutorPtr& executor) {
    nodes_.emplace_back(executor);
    return nodes_.size() - 1;
  }

  bool hasCycle() {
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
    std::vector<Key> keys;
    for (Key key = 0; key < nodes_.size(); key++) {
      if (!nodes_[key].hasPrev) {
        keys.push_back(key);
      }
    }
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
    for (auto& edges : nexts) {
      if (!edges.empty()) {
        return true;
      }
    }
    return false;
  }

  std::vector<Node> nodes_;
};

}

