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

#include <atomic>
#include <thread>
#include <vector>

#include "raster/concurrency/ThreadPoolExecutor.h"
#include "accelerator/thread/Synchronized.h"

namespace rdd {

class PeriodicScheduler {
 public:
  PeriodicScheduler(std::shared_ptr<ThreadPoolExecutor> executor);

  void start();

  void stop();

  void add(acc::VoidFunc&& func, uint64_t interval);

 private:
  struct PeriodicTask {
    acc::VoidFunc func;
    uint64_t interval;
    mutable uint64_t stamp;

    PeriodicTask(acc::VoidFunc&& func_, uint64_t interval_)
      : func(std::move(func_)),
        interval(interval_),
        stamp(0) {}

    bool checkStamp(uint64_t st) const;
  };

  void run();

  void runTasks();

  std::shared_ptr<ThreadPoolExecutor> executor_;
  std::thread handle_;
  acc::Synchronized<std::vector<PeriodicTask>> tasks_;
  std::atomic<bool> running_{false};
  uint64_t timestamp_{0};
};

} // namespace rdd
