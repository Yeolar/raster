/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace rdd {

template <class T>
class DAG {
public:
  struct Node {
    T id;
    std::set<T> next;
    std::set<T> prev;
  };

  DAG() {}

  void addNode(const T& id, const std::vector<T>& next) {
    auto& node = graph_[id];
    node.id = id;
    for (auto& i : next) {
      node.next.insert(i);
      graph_[i].prev.insert(id);
    }
  }

  bool checkValid() const {
    T b, e;
    try {
      b = begin();
      e = end();
    } catch (std::exception& e) {
      return false;
    }
    return !hasCycle(b, {});
  }

  T begin() const {
    std::vector<T> v;
    for (auto& kv : graph_) {
      if (kv.second.prev.empty()) {
        v.push_back(kv.first);
      }
    }
    if (v.size() != 1) {
      throw std::logic_error("not single first node");
    }
    return v[0];
  }

  T end() const {
    std::vector<T> v;
    for (auto& kv : graph_) {
      if (kv.second.next.empty()) {
        v.push_back(kv.first);
      }
    }
    if (v.size() != 1) {
      throw std::logic_error("not single last node");
    }
    return v[0];
  }

  std::set<T> next(const T& id) const {
    return graph_.at(id).next;
  }

  std::set<T> prev(const T& id) const {
    return graph_.at(id).prev;
  }

private:
  bool hasCycle(const T& id, const std::set<T>& v) const {
    for (auto& i : next(id)) {
      auto vv = v;
      if (!vv.insert(i).second || hasCycle(i, vv)) {
        return true;
      }
    }
    return false;
  }

  std::map<T, Node> graph_;
};

}

