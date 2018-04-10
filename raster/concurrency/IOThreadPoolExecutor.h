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

#include <atomic>

#include "raster/concurrency/ThreadPoolExecutor.h"
#include "raster/event/EventLoopManager.h"

namespace rdd {

/**
 * A Thread Pool for IO bound tasks
 *
 * @note For this thread pool, stop() behaves like join() because
 * outstanding tasks belong to the event base and will be executed upon its
 * destruction.
 */
class IOThreadPoolExecutor : public ThreadPoolExecutor {
 public:
  explicit IOThreadPoolExecutor(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("IOThreadPool"),
      EventLoopManager* ebm = EventLoopManager::get());

  ~IOThreadPoolExecutor() override;

  void add(acc::VoidFunc func) override;
  void add(acc::VoidFunc func,
           uint64_t expiration,
           acc::VoidFunc expireCallback = nullptr) override;

  uint64_t getPendingTaskCount() override;

  EventLoop* getEventLoop();

  static EventLoop* getEventLoop(ThreadPoolExecutor::ThreadHandle*);

  EventLoopManager* getEventLoopManager();

 private:
  struct ACC_ALIGN_TO_AVOID_FALSE_SHARING IOThread : public Thread {
    IOThread(IOThreadPoolExecutor* pool)
        : Thread(pool), shouldRun(true), pendingTasks(0) {}
    std::atomic<bool> shouldRun;
    std::atomic<size_t> pendingTasks;
    EventLoop* eventLoop;
    std::mutex eventLoopShutdownMutex_;
  };

  ThreadPtr makeThread() override;
  std::shared_ptr<IOThread> pickThread();
  void threadRun(ThreadPtr thread) override;
  void stopThreads(size_t n) override;

  std::atomic<size_t> nextThread_;
  acc::ThreadLocal<std::shared_ptr<IOThread>> thisThread_;
  EventLoopManager* eventLoopManager_;
};

} // namespace rdd
