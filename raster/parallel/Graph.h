/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace rdd {

struct Graph {
  typedef std::vector<std::string> Outs;

  std::map<std::string, Outs> nodes;

  void set(const std::string& node, const Outs& outs) {
    nodes[node] = outs;
  }

  void add(const std::string& node, const std::string& next) {
    nodes[node].push_back(next);
  }

  Outs get(const std::string& node) const {
    return nodes.at(node);
  }
};

class GraphManager {
 public:
  GraphManager() {}

  Graph& graph(const std::string& key) {
    return graphs_[key];
  }

 private:
  std::map<std::string, Graph> graphs_;
};

} // namespace rdd
