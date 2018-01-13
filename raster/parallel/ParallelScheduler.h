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
#include <vector>

#include "raster/parallel/DAG.h"
#include "raster/parallel/Graph.h"
#include "raster/thread/Waiter.h"

namespace rdd {

class JobBase {
 public:
  virtual ~JobBase() {}
  virtual void run() = 0;

  void setName(const std::string& name) { name_ = name; }
  std::string name() const { return name_; }

 protected:
  std::string name_;
};

class ParallelScheduler {
 public:
  explicit ParallelScheduler(std::shared_ptr<ThreadPoolExecutor> executor);

  ParallelScheduler(std::shared_ptr<ThreadPoolExecutor> executor,
                    const Graph& graph);

  void add(const std::string& name,
           const std::vector<std::string>& next,
           VoidFunc&& func);

  void add(const std::string& name,
           const std::vector<std::string>& next,
           JobBase* job);

  void add(const std::string& name,
           const std::vector<std::string>& next);

  void run(bool blocking = false);

 private:
  void setDependency();

  DAG dag_;
  Graph graph_;
  std::map<std::string, DAG::Key> map_;
  bool setup_{false};
  Waiter waiter_;
};

} // namespace rdd
