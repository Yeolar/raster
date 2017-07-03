/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/ThreadFactory.h"
#include "rddoc/util/Baton.h"
#include "rddoc/util/BlockingQueue.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/Memory.h"
#include "rddoc/util/RWSpinLock.h"

namespace rdd {

struct Thread {
  explicit Thread(bool bindCpu_)
    : id(nextId++), handle(), bindCpu(bindCpu_), idle(true) {}

  virtual ~Thread() {}

  static std::atomic<uint64_t> nextId;

  uint64_t id;
  std::thread handle;
  bool bindCpu;
  bool idle;
  Baton startupBaton;
};

typedef std::shared_ptr<Thread> ThreadPtr;

inline bool operator< (const ThreadPtr& lhs, const ThreadPtr& rhs) {
  return lhs->id < rhs->id;
}

struct Task {
  struct Stats {
    Stats() : expired(false), waitTime(0), runTime(0) {}
    bool expired;
    uint64_t waitTime;
    uint64_t runTime;
  };

  explicit Task(VoidFunc&& func,
                uint64_t expiration,
                VoidFunc&& expireCallback)
    : func_(std::move(func)),
      enqueueTime_(timestampNow()),
      expiration_(expiration),
      expireCallback_(std::move(expireCallback)) {}

  VoidFunc func_;
  Stats stats_;
  uint64_t enqueueTime_;
  uint64_t expiration_;
  VoidFunc expireCallback_;
};

class ThreadPool {
public:
  explicit ThreadPool(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory,
      bool bindCpu)
    : threadFactory_(std::move(threadFactory)), bindCpu_(bindCpu) {}

  virtual ~ThreadPool() {}

  virtual void add(VoidFunc func) = 0;
  virtual void add(VoidFunc func,
                   uint64_t expiration,
                   VoidFunc expireCallback) = 0;

  void setThreadFactory(std::shared_ptr<ThreadFactory> threadFactory) {
    RDDCHECK(numThreads() == 0);
    threadFactory_ = std::move(threadFactory);
  }

  std::shared_ptr<ThreadFactory> getThreadFactory() {
    return threadFactory_;
  }

  size_t numThreads();
  void setNumThreads(size_t numThreads);

  void stop();
  void join();

  struct Stats {
    Stats() : threadCount(0), idleThreadCount(0), activeThreadCount(0),
              pendingTaskCount(0), totalTaskCount(0) {}
    size_t threadCount, idleThreadCount, activeThreadCount;
    size_t pendingTaskCount, totalTaskCount;
  };

  Stats getStats();

  class Observer {
  public:
    virtual void threadStarted(Thread*) = 0;
    virtual void threadStopped(Thread*) = 0;
    virtual void threadPreviouslyStarted(Thread* h) {
      threadStarted(h);
    }
    virtual void threadNotYetStopped(Thread* h) {
      threadStopped(h);
    }
  };

  void addObserver(std::shared_ptr<Observer>);
  void removeObserver(std::shared_ptr<Observer>);

protected:
  void addThreads(size_t n);
  void removeThreads(size_t n, bool isJoin);

  virtual ThreadPtr makeThread() {
    return std::make_shared<Thread>(bindCpu_);
  }

  static void runTask(const ThreadPtr& thread, Task&& task);

  virtual void threadRun(ThreadPtr thread) = 0;
  virtual void stopThreads(size_t n) = 0;
  virtual size_t getPendingTaskCount() = 0;

  std::shared_ptr<ThreadFactory> threadFactory_;
  bool bindCpu_;

  std::vector<ThreadPtr> threads_;
  RWSpinLock threadsLock_;
  GenericBlockingQueue<ThreadPtr> stoppedThreads_;
  std::atomic<bool> isJoin_; // whether the current downsizing is a join

  std::vector<std::shared_ptr<Observer>> observers_;
};

}
