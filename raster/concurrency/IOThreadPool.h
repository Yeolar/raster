/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/concurrency/ThreadPool.h"
#include "raster/io/event/EventLoop.h"
#include "raster/io/event/EventLoopManager.h"

namespace rdd {

/**
 * A Thread Pool for IO bound tasks
 */
class IOThreadPool : public ThreadPool {
public:
  explicit IOThreadPool(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<ThreadFactory>("IOThreadPool"),
      bool bindCpu = false,
      EventLoopManager* manager = EventLoopManager::get());

  ~IOThreadPool();

  void add(VoidFunc func);
  void add(VoidFunc func,
           uint64_t expiration,
           VoidFunc expireCallback = nullptr);

  static EventLoop* getEventLoop(Thread* thread);

  EventLoop* getEventLoop();

  EventLoopManager* getEventLoopManager();

private:
  struct IOThread : public Thread {
    IOThread(ThreadPool* pool, bool bindCpu_)
      : Thread(pool, bindCpu_),
        shouldRun(true),
        pendingTasks(0) {}

    std::atomic<bool> shouldRun;
    std::atomic<size_t> pendingTasks;
    EventLoop* eventLoop;
    std::mutex eventLoopShutdownLock_;
  };

  ThreadPtr makeThread() {
    return std::make_shared<IOThread>(this, bindCpu_);
  }

  std::shared_ptr<IOThread> pickThread();

  void threadRun(ThreadPtr thread);
  void stopThreads(size_t n);
  size_t getPendingTaskCount();

  size_t nextThread_;
  ThreadLocal<std::shared_ptr<IOThread>> thisThread_;
  EventLoopManager* eventLoopManager_;
};

} // namespace rdd
