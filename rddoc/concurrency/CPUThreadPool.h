/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/ThreadPool.h"

namespace rdd {

struct CPUTask : public Task {
  explicit CPUTask(
      VoidFunc&& f,
      uint64_t expiration,
      VoidFunc&& expireCallback)
    : Task(std::move(f), expiration, std::move(expireCallback)),
      poison(false) {}

  CPUTask()
    : Task(nullptr, 0, nullptr),
      poison(true) {}

  bool poison;
};

/**
 * A Thread pool for CPU bound tasks.
 */
class CPUThreadPool : public ThreadPool {
public:
  explicit CPUThreadPool(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("CPUThreadPool"),
      bool bindCpu = false);

  ~CPUThreadPool();

  void add(VoidFunc func);
  void add(VoidFunc func,
           uint64_t expiration,
           VoidFunc expireCallback = nullptr);

private:
  void threadRun(ThreadPtr thread);
  void stopThreads(size_t n);
  size_t getPendingTaskCount();

  GenericBlockingQueue<CPUTask> taskQueue_;
  std::atomic<ssize_t> threadsToStop_{0};
};

}
