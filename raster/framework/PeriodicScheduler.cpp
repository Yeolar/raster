/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/PeriodicScheduler.h"
#include "raster/util/Time.h"

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

void PeriodicScheduler::add(VoidFunc&& func, uint64_t interval) {
  tasks_.wlock()->emplace_back(std::move(func), interval);
}

void PeriodicScheduler::run() {
  setCurrentThreadName("AutoTaskThread");
  timestamp_ = timestampNow();
  while (running_) {
    runTasks();
    usleep(10000); // 10ms
  }
}

void PeriodicScheduler::runTasks() {
  uint64_t passed = timePassed(timestamp_);
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
