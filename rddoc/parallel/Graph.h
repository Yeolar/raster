/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace rdd {

template <class T>
struct Graph {

  struct Edge {
    T node;
    std::vector<T> next;
  };

  typedef typename std::vector<Edge>::iterator iterator;
  typedef typename std::vector<Edge>::const_iterator const_iterator;

  std::vector<Edge> graph;

  iterator begin() { return graph.begin(); }
  const_iterator begin() const { return graph.begin(); }

  iterator end() { return graph.end(); }
  const_iterator end() const { return graph.end(); }

  void add(const T& node, const std::vector<T>& next) {
    graph.emplace_back(node, next);
  }

  Edge get(const T& node) const {
    for (auto& p : graph) {
      if (p.node == node) {
        return p;
      }
    }
    throw std::overflow_error(node);
  }
};

}
