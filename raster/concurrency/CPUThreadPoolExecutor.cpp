/*
 * Copyright 2017 Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/concurrency/CPUThreadPoolExecutor.h"

namespace raster {

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
          std::make_unique<MPMCBlockingQueue<CPUTask>>(
              CPUThreadPoolExecutor::kDefaultMaxQueueSize),
          std::move(threadFactory)) {}

CPUThreadPoolExecutor::CPUThreadPoolExecutor(
    size_t numThreads,
    size_t maxQueueSize,
    std::shared_ptr<ThreadFactory> threadFactory)
    : CPUThreadPoolExecutor(
          numThreads,
          std::make_unique<MPMCBlockingQueue<CPUTask>>(
              maxQueueSize),
          std::move(threadFactory)) {}

CPUThreadPoolExecutor::~CPUThreadPoolExecutor() {
  stop();
  ACCCHECK(threadsToStop_ == 0);
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
    if (ACC_UNLIKELY(task.poison)) {
      ACCCHECK(threadsToStop_-- > 0);
      for (auto& o : observers_) {
        o->threadStopped(thread.get());
      }
      acc::RWSpinLock::WriteHolder w{&threadListLock_};
      threadList_.remove(thread);
      stoppedThreads_.add(thread);
      return;
    } else {
      runTask(thread, std::move(task));
    }

    if (ACC_UNLIKELY(threadsToStop_ > 0 && !isJoin_)) {
      if (--threadsToStop_ >= 0) {
        acc::RWSpinLock::WriteHolder w{&threadListLock_};
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

} // namespace raster
