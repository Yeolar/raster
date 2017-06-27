/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/ThreadPool.h"
#include "rddoc/io/event/EventLoop.h"
#include "rddoc/io/event/EventLoopManager.h"

namespace rdd {

struct IOThread : public Thread {
  IOThread(bool bindCpu_)
    : Thread(bindCpu_), shouldRun(true), pendingTasks(0) {}

  std::atomic<bool> shouldRun;
  std::atomic<size_t> pendingTasks;
  EventLoop* eventLoop;
  Lock eventLoopShutdownLock_;
};

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
  ThreadPtr makeThread() {
    return std::make_shared<IOThread>(bindCpu_);
  }

  std::shared_ptr<IOThread> pickThread();

  void threadRun(ThreadPtr thread);
  void stopThreads(size_t n);
  size_t getPendingTaskCount();

  size_t nextThread_;
  ThreadLocal<std::shared_ptr<IOThread>> thisThread_;
  EventLoopManager* eventLoopManager_;
};

}
