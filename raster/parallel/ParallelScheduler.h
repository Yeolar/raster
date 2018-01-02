/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>

#include "raster/parallel/DAG.h"
#include "raster/parallel/Graph.h"

namespace rdd {

class JobBase {
 public:
  virtual ~JobBase() {}
  virtual void run() = 0;

  void setName(const std::string& name) { name_ = name; }
  std::string name() const { return name_; }

 private:
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

  void run(VoidFunc&& finishCallback);

 private:
  void setDependency();

  DAG dag_;
  Graph graph_;
  std::map<std::string, DAG::Key> map_;
  bool setup_{false};
};

} // namespace rdd
