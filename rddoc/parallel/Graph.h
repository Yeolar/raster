/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace rdd {

struct Graph {

  struct Edge {
    std::string node;
    std::vector<std::string> next;

    Edge(const std::string& node_, const std::vector<std::string>& next_)
      : node(node_), next(next_) {}
  };

  typedef typename std::vector<Edge>::iterator iterator;
  typedef typename std::vector<Edge>::const_iterator const_iterator;

  std::vector<Edge> graph;

  iterator begin() { return graph.begin(); }
  const_iterator begin() const { return graph.begin(); }

  iterator end() { return graph.end(); }
  const_iterator end() const { return graph.end(); }

  void add(const std::string& node, const std::vector<std::string>& next) {
    graph.emplace_back(node, next);
  }

  Edge get(const std::string& node) const {
    for (auto& p : graph) {
      if (p.node == node) {
        return p;
      }
    }
    throw std::overflow_error(node);
  }
};

class GraphManager {
public:
  GraphManager() {}

  Graph& getGraph(const std::string& key) {
    return graphs_[key];
  }

private:
  std::map<std::string, Graph> graphs_;
};

}
