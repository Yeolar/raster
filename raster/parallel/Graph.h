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
