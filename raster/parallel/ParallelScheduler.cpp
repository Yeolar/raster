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

void ParallelScheduler::run(bool blocking) {
  if (!dag_.empty()) {
    setDependency();
    if (blocking) {
      dag_.go([&]() { waiter_.notify(); });
    } else {
      dag_.go(nullptr);
    }
    waiter_.wait();
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
