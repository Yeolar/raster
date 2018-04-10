/*
 * Copyright 2017 Facebook, Inc.
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
      std::unique_ptr<acc::BlockingQueue<CPUTask>> taskQueue,
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

  void add(acc::VoidFunc func) override;
  void add(acc::VoidFunc func,
           uint64_t expiration,
           acc::VoidFunc expireCallback = nullptr) override;

  uint64_t getPendingTaskCount() override;

  struct CPUTask : public ThreadPoolExecutor::Task {
    // Must be noexcept move constructible so it can be used in MPMCQueue

    explicit CPUTask(
        acc::VoidFunc&& f,
        uint64_t expiration,
        acc::VoidFunc&& expireCallback)
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

  std::unique_ptr<acc::BlockingQueue<CPUTask>> taskQueue_;
  std::atomic<ssize_t> threadsToStop_{0};
};

} // namespace rdd
