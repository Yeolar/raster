/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/concurrency/Executor.h"
#include "raster/concurrency/ThreadFactory.h"
#include "raster/thread/Baton.h"
#include "raster/thread/BlockingQueue.h"
#include "raster/thread/RWSpinLock.h"
#include "raster/thread/Synchronized.h"
#include "raster/util/Logging.h"

namespace rdd {

class ThreadPoolExecutor : public virtual Executor {
 public:
  explicit ThreadPoolExecutor(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory,
      bool isWaitForAll = false);

  ~ThreadPoolExecutor() override;

  void add(VoidFunc func) override = 0;
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

  /*
   * stop() is best effort - there is no guarantee that unexecuted tasks won't
   * be executed before it returns. Specifically, IOThreadPoolExecutor's stop()
   * behaves like join().
   */
  void stop();
  void join();

  struct PoolStats {
    PoolStats()
        : threadCount(0),
          idleThreadCount(0),
          activeThreadCount(0),
          pendingTaskCount(0),
          totalTaskCount(0),
          maxIdleTime(0) {}
    size_t threadCount, idleThreadCount, activeThreadCount;
    size_t pendingTaskCount, totalTaskCount;
    uint64_t maxIdleTime;
  };

  PoolStats getPoolStats();
  virtual uint64_t getPendingTaskCount() = 0;

  struct TaskStats {
    TaskStats() : expired(false), waitTime(0), runTime(0) {}
    bool expired;
    uint64_t waitTime;
    uint64_t runTime;
  };

  using TaskStatsCallback = std::function<void(TaskStats)>;
  void subscribeToTaskStats(TaskStatsCallback cb);

  /**
   * Base class for threads created with ThreadPoolExecutor.
   * Some subclasses have methods that operate on these
   * handles.
   */
  struct ThreadHandle {
    virtual ~ThreadHandle() = default;
  };

  /**
   * Observer interface for thread start/stop.
   * Provides hooks so actions can be taken when
   * threads are created
   */
  class Observer {
   public:
    virtual void threadStarted(ThreadHandle*) = 0;
    virtual void threadStopped(ThreadHandle*) = 0;
    virtual void threadPreviouslyStarted(ThreadHandle* h) {
      threadStarted(h);
    }
    virtual void threadNotYetStopped(ThreadHandle* h) {
      threadStopped(h);
    }
    virtual ~Observer() = default;
  };

  void addObserver(std::shared_ptr<Observer>);
  void removeObserver(std::shared_ptr<Observer>);

 protected:
  // Prerequisite: threadListLock_ writelocked
  void addThreads(size_t n);
  // Prerequisite: threadListLock_ writelocked
  void removeThreads(size_t n, bool isJoin);

  struct TaskStatsCallbackRegistry;

  struct RDD_ALIGN_TO_AVOID_FALSE_SHARING Thread : public ThreadHandle {
    explicit Thread(ThreadPoolExecutor* pool)
        : id(nextId++),
          handle(),
          idle(true),
          lastActiveTime(timestampNow()),
          taskStatsCallbacks(pool->taskStatsCallbacks_) {}

    ~Thread() override = default;

    static std::atomic<uint64_t> nextId;
    uint64_t id;
    std::thread handle;
    bool idle;
    uint64_t lastActiveTime;
    Baton startupBaton;
    std::shared_ptr<TaskStatsCallbackRegistry> taskStatsCallbacks;
  };

  typedef std::shared_ptr<Thread> ThreadPtr;

  struct Task {
    explicit Task(
        VoidFunc&& func,
        uint64_t expiration,
        VoidFunc&& expireCallback);
    VoidFunc func_;
    TaskStats stats_;
    uint64_t enqueueTime_;
    uint64_t expiration_;
    VoidFunc expireCallback_;
  };

  static void runTask(const ThreadPtr& thread, Task&& task);

  // The function that will be bound to pool threads. It must call
  // thread->startupBaton.post() when it's ready to consume work.
  virtual void threadRun(ThreadPtr thread) = 0;

  // Stop n threads and put their ThreadPtrs in the stoppedThreads_ queue
  // and remove them from threadList_, either synchronize or asynchronize
  // Prerequisite: threadListLock_ writelocked
  virtual void stopThreads(size_t n) = 0;

  // Join n stopped threads and remove them from waitingForJoinThreads_ queue.
  // Should not hold a lock because joining thread operation may invoke some
  // cleanup operations on the thread, and those cleanup operations may
  // require a lock on ThreadPoolExecutor.
  void joinStoppedThreads(size_t n);

  // Create a suitable Thread struct
  virtual ThreadPtr makeThread() {
    return std::make_shared<Thread>(this);
  }

  class ThreadList {
   public:
    void add(const ThreadPtr& state) {
      auto it = std::lower_bound(
          vec_.begin(),
          vec_.end(),
          state,
          // compare method is a static method of class
          // and therefore cannot be inlined by compiler
          // as a template predicate of the STL algorithm
          // but wrapped up with the lambda function (lambda will be inlined)
          // compiler can inline compare method as well
          [&](const ThreadPtr& ts1, const ThreadPtr& ts2) -> bool { // inline
            return compare(ts1, ts2);
          });
      vec_.insert(it, state);
    }

    void remove(const ThreadPtr& state) {
      auto itPair = std::equal_range(
          vec_.begin(),
          vec_.end(),
          state,
          // the same as above
          [&](const ThreadPtr& ts1, const ThreadPtr& ts2) -> bool { // inline
            return compare(ts1, ts2);
          });
      RDDCHECK(itPair.first != vec_.end());
      RDDCHECK(std::next(itPair.first) == itPair.second);
      vec_.erase(itPair.first);
    }

    const std::vector<ThreadPtr>& get() const {
      return vec_;
    }

   private:
    static bool compare(const ThreadPtr& ts1, const ThreadPtr& ts2) {
      return ts1->id < ts2->id;
    }

    std::vector<ThreadPtr> vec_;
  };

  std::shared_ptr<ThreadFactory> threadFactory_;
  const bool isWaitForAll_; // whether to wait till event base loop exits

  ThreadList threadList_;
  RWSpinLock threadListLock_;
  GenericBlockingQueue<ThreadPtr> stoppedThreads_;
  std::atomic<bool> isJoin_; // whether the current downsizing is a join

  struct TaskStatsCallbackRegistry {
    ThreadLocal<bool> inCallback;
    Synchronized<std::vector<TaskStatsCallback>> callbackList;
  };
  std::shared_ptr<TaskStatsCallbackRegistry> taskStatsCallbacks_;
  std::vector<std::shared_ptr<Observer>> observers_;
};

} // namespace rdd
