/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/concurrency/ThreadPoolExecutor.h"

namespace rdd {

/**
 * A Thread pool for CPU bound tasks.
 *
 * @note The default queue throws when full, so add() can fail.
 * Furthermore, join() can also fail if the queue is full,
 * because it enqueues numThreads poison tasks to stop the threads.
 *
 * @note stop() will finish all outstanding tasks at exit.
 */
class CPUThreadPoolExecutor : public ThreadPoolExecutor {
 public:
  struct CPUTask;

  CPUThreadPoolExecutor(
      size_t numThreads,
      std::unique_ptr<BlockingQueue<CPUTask>> taskQueue,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("CPUThreadPool"));

  explicit CPUThreadPoolExecutor(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("CPUThreadPool"));

  CPUThreadPoolExecutor(
      size_t numThreads,
      size_t maxQueueSize,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("CPUThreadPool"));

  ~CPUThreadPoolExecutor() override;

  void add(VoidFunc func) override;
  void add(VoidFunc func,
           uint64_t expiration,
           VoidFunc expireCallback = nullptr) override;

  uint64_t getPendingTaskCount() override;

  struct CPUTask : public ThreadPoolExecutor::Task {
    // Must be noexcept move constructible so it can be used in MPMCQueue

    explicit CPUTask(
        VoidFunc&& f,
        uint64_t expiration,
        VoidFunc&& expireCallback)
        : Task(std::move(f), expiration, std::move(expireCallback)),
          poison(false) {}
    CPUTask()
        : Task(nullptr, 0, nullptr), poison(true) {}

    bool poison;
  };

  static const size_t kDefaultMaxQueueSize;

 private:
  void threadRun(ThreadPtr thread) override;
  void stopThreads(size_t n) override;

  std::unique_ptr<BlockingQueue<CPUTask>> taskQueue_;
  std::atomic<ssize_t> threadsToStop_{0};
};

} // namespace rdd
