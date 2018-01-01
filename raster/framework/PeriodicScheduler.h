/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "raster/concurrency/ThreadPoolExecutor.h"
#include "raster/thread/Synchronized.h"

namespace rdd {

class PeriodicScheduler {
 public:
  PeriodicScheduler(std::shared_ptr<ThreadPoolExecutor> executor);

  void start();

  void stop();

  void add(VoidFunc&& func, uint64_t interval);

 private:
  struct PeriodicTask {
    VoidFunc func;
    uint64_t interval;
    mutable uint64_t stamp;

    PeriodicTask(VoidFunc&& func_, uint64_t interval_)
      : func(std::move(func_)),
        interval(interval_),
        stamp(0) {}

    bool checkStamp(uint64_t st) const;
  };

  void run();

  void runTasks();

  std::shared_ptr<ThreadPoolExecutor> executor_;
  std::thread handle_;
  Synchronized<std::vector<PeriodicTask>> tasks_;
  std::atomic<bool> running_{false};
  uint64_t timestamp_{0};
};

} // namespace rdd
