/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/parallel/ParallelScheduler.h"

#include "raster/util/ReflectObject.h"

namespace rdd {

ParallelScheduler::ParallelScheduler(
    std::shared_ptr<ThreadPoolExecutor> executor)
  : dag_(executor) {}

ParallelScheduler::ParallelScheduler(
    std::shared_ptr<ThreadPoolExecutor> executor,
    const Graph& graph)
  : dag_(executor), graph_(graph) {}

void ParallelScheduler::add(const std::string& name,
                            const std::vector<std::string>& next,
                            VoidFunc&& func) {
  map_[name] = dag_.add(std::move(func));
  graph_.set(name, next);
}

void ParallelScheduler::add(const std::string& name,
                            const std::vector<std::string>& next,
                            JobBase* job) {
  job->setName(name);
  add(name, next, [&]() { job->run(); });
}

void ParallelScheduler::add(const std::string& name,
                            const std::vector<std::string>& next) {
  auto cls = name.substr(0, name.find(':'));
  auto job = makeReflectObject<JobBase>(cls.c_str());
  add(name, next, job);
}

void ParallelScheduler::run(VoidFunc&& finishCallback) {
  if (!dag_.empty()) {
    setDependency();
    dag_.go(std::move(finishCallback));
  }
}

void ParallelScheduler::setDependency() {
  for (auto& kv : graph_.nodes) {
    for (auto& next : kv.second) {
      dag_.dependency(map_[kv.first], map_[next]);
    }
  }
  setup_ = true;
}

} // namespace rdd
