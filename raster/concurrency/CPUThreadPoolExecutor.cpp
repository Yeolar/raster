/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/concurrency/CPUThreadPoolExecutor.h"
#include "raster/thread/BlockingQueue.h"

namespace rdd {

const size_t CPUThreadPoolExecutor::kDefaultMaxQueueSize = 1 << 14;

CPUThreadPoolExecutor::CPUThreadPoolExecutor(
    size_t numThreads,
    std::unique_ptr<BlockingQueue<CPUTask>> taskQueue,
    std::shared_ptr<ThreadFactory> threadFactory)
    : ThreadPoolExecutor(numThreads, std::move(threadFactory)),
      taskQueue_(std::move(taskQueue)) {
  setNumThreads(numThreads);
}

CPUThreadPoolExecutor::CPUThreadPoolExecutor(
    size_t numThreads,
    std::shared_ptr<ThreadFactory> threadFactory)
    : CPUThreadPoolExecutor(
          numThreads,
          make_unique<MPMCBlockingQueue<CPUTask>>(
              CPUThreadPoolExecutor::kDefaultMaxQueueSize),
          std::move(threadFactory)) {}

CPUThreadPoolExecutor::CPUThreadPoolExecutor(
    size_t numThreads,
    size_t maxQueueSize,
    std::shared_ptr<ThreadFactory> threadFactory)
    : CPUThreadPoolExecutor(
          numThreads,
          make_unique<MPMCBlockingQueue<CPUTask>>(
              maxQueueSize),
          std::move(threadFactory)) {}

CPUThreadPoolExecutor::~CPUThreadPoolExecutor() {
  stop();
  RDDCHECK(threadsToStop_ == 0);
}

void CPUThreadPoolExecutor::add(VoidFunc func) {
  add(std::move(func), 0);
}

void CPUThreadPoolExecutor::add(
    VoidFunc func,
    uint64_t expiration,
    VoidFunc expireCallback) {
  // TODO handle enqueue failure, here and in other add() callsites
  taskQueue_->add(
      CPUTask(std::move(func), expiration, std::move(expireCallback)));
}

void CPUThreadPoolExecutor::threadRun(std::shared_ptr<Thread> thread) {
  thread->startupBaton.post();
  while (true) {
    auto task = taskQueue_->take();
    if (UNLIKELY(task.poison)) {
      RDDCHECK(threadsToStop_-- > 0);
      for (auto& o : observers_) {
        o->threadStopped(thread.get());
      }
      rdd::RWSpinLock::WriteHolder w{&threadListLock_};
      threadList_.remove(thread);
      stoppedThreads_.add(thread);
      return;
    } else {
      runTask(thread, std::move(task));
    }

    if (UNLIKELY(threadsToStop_ > 0 && !isJoin_)) {
      if (--threadsToStop_ >= 0) {
        rdd::RWSpinLock::WriteHolder w{&threadListLock_};
        threadList_.remove(thread);
        stoppedThreads_.add(thread);
        return;
      } else {
        threadsToStop_++;
      }
    }
  }
}

void CPUThreadPoolExecutor::stopThreads(size_t n) {
  threadsToStop_ += n;
  for (size_t i = 0; i < n; i++) {
    taskQueue_->add(CPUTask());
  }
}

uint64_t CPUThreadPoolExecutor::getPendingTaskCount() {
  return taskQueue_->size();
}

} // namespace rdd
