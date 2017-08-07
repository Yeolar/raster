/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/concurrency/CPUThreadPool.h"

namespace rdd {

CPUThreadPool::CPUThreadPool(
    size_t numThreads,
    std::shared_ptr<ThreadFactory> threadFactory,
    bool bindCpu)
  : ThreadPool(numThreads, std::move(threadFactory), bindCpu) {
  addThreads(numThreads);
  RDDCHECK(threads_.size() == numThreads);
}

CPUThreadPool::~CPUThreadPool() {
  stop();
}

void CPUThreadPool::add(VoidFunc func) {
  add(std::move(func), 0);
}

void CPUThreadPool::add(VoidFunc func,
                        uint64_t expiration,
                        VoidFunc expireCallback) {
  taskQueue_.add(
      CPUTask(std::move(func), expiration, std::move(expireCallback)));
}

void CPUThreadPool::threadRun(ThreadPtr thread) {
  if (thread->bindCpu) {
    setCpuAffinity(thread->id);
  }
  thread->startupBaton.post();
  while (1) {
    auto task = taskQueue_.take();
    if (UNLIKELY(task.poison)) {
      RDDCHECK(threadsToStop_-- > 0);
      for (auto& o : observers_) {
        o->threadStopped(thread.get());
      }
      stoppedThreads_.add(thread);
      return;
    } else {
      runTask(thread, std::move(task));
    }
    if (UNLIKELY(threadsToStop_ > 0 && !isJoin_)) {
      if (--threadsToStop_ >= 0) {
        stoppedThreads_.add(thread);
        return;
      } else {
        threadsToStop_++;
      }
    }
  }
}

void CPUThreadPool::stopThreads(size_t n) {
  RDDCHECK(stoppedThreads_.size() == 0);
  threadsToStop_ = n;
  for (size_t i = 0; i < n; i++) {
    taskQueue_.add(CPUTask());
  }
}

size_t CPUThreadPool::getPendingTaskCount() {
  return taskQueue_.size();
}

} // namespace rdd
