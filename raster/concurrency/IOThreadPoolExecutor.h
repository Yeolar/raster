/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>

#include "raster/concurrency/ThreadPoolExecutor.h"
#include "raster/io/event/EventLoopManager.h"

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
      EventLoopManager* ebm = EventLoopManager::get(),
      bool waitForAll = false);

  ~IOThreadPoolExecutor() override;

  void add(VoidFunc func) override;
  void add(VoidFunc func,
           uint64_t expiration,
           VoidFunc expireCallback = nullptr) override;

  uint64_t getPendingTaskCount() override;

  EventLoop* getEventLoop();

  static EventLoop* getEventLoop(ThreadPoolExecutor::ThreadHandle*);

  EventLoopManager* getEventLoopManager();

 private:
  struct RDD_ALIGN_TO_AVOID_FALSE_SHARING IOThread : public Thread {
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
  ThreadLocal<std::shared_ptr<IOThread>> thisThread_;
  EventLoopManager* eventLoopManager_;
};

} // namespace rdd
