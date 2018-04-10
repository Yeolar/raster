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

#include "raster/framework/PeriodicScheduler.h"

#include "accelerator/Time.h"

namespace rdd {

PeriodicScheduler::PeriodicScheduler(
    std::shared_ptr<ThreadPoolExecutor> executor)
  : executor_(executor) {
}

void PeriodicScheduler::start() {
  running_ = true;
  handle_ = std::thread(&PeriodicScheduler::run, this);
}

void PeriodicScheduler::stop() {
  running_ = false;
  handle_.join();
}

void PeriodicScheduler::add(acc::VoidFunc&& func, uint64_t interval) {
  tasks_.wlock()->emplace_back(std::move(func), interval);
}

void PeriodicScheduler::run() {
  acc::setCurrentThreadName("AutoTaskThread");
  timestamp_ = acc::timestampNow();
  while (running_) {
    runTasks();
    usleep(10000); // 10ms
  }
}

void PeriodicScheduler::runTasks() {
  uint64_t passed = acc::timePassed(timestamp_);
  auto tasks = tasks_.rlock();
  for (const auto& t : *tasks) {
    if (t.checkStamp(passed / t.interval)) {
      executor_->add(t.func);
    }
  }
}

bool PeriodicScheduler::PeriodicTask::checkStamp(uint64_t st) const {
  return st >= stamp ? (stamp = st + 1) : false;
}

} // namespace rdd
