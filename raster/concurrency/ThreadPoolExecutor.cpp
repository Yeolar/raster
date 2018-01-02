/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/concurrency/ThreadPoolExecutor.h"

#include "raster/util/ScopeGuard.h"

namespace rdd {

ThreadPoolExecutor::ThreadPoolExecutor(
    size_t /* numThreads */,
    std::shared_ptr<ThreadFactory> threadFactory)
    : threadFactory_(std::move(threadFactory)),
      taskStatsCallbacks_(std::make_shared<TaskStatsCallbackRegistry>()) {}

ThreadPoolExecutor::~ThreadPoolExecutor() {
  RDDCHECK_EQ(0, threadList_.get().size());
}

ThreadPoolExecutor::Task::Task(
    VoidFunc&& func,
    uint64_t expiration,
    VoidFunc&& expireCallback)
    : func_(std::move(func)),
      expiration_(expiration),
      expireCallback_(std::move(expireCallback)) {
  // Assume that the task in enqueued on creation
  enqueueTime_ = timestampNow();
}

ThreadPoolExecutor::Task::Task(Task&& other) noexcept
    : func_(std::move(other.func_)),
      stats_(other.stats_),
      enqueueTime_(other.enqueueTime_),
      expiration_(other.expiration_),
      expireCallback_(std::move(other.expireCallback_)) {
}

ThreadPoolExecutor::Task&
ThreadPoolExecutor::Task::operator=(Task&& other) noexcept {
  func_.swap(other.func_);
  stats_ = other.stats_;
  enqueueTime_ = other.enqueueTime_;
  expiration_ = other.expiration_;
  expireCallback_.swap(other.expireCallback_);
  return *this;
}

void ThreadPoolExecutor::runTask(const ThreadPtr& thread, Task&& task) {
  thread->idle = false;
  auto startTime = timestampNow();
  task.stats_.waitTime = startTime - task.enqueueTime_;
  if (task.expiration_ > 0 && task.stats_.waitTime >= task.expiration_) {
    task.stats_.expired = true;
    if (task.expireCallback_ != nullptr) {
      task.expireCallback_();
    }
  } else {
    try {
      task.func_();
    } catch (const std::exception& e) {
      RDDLOG(ERROR) << "ThreadPoolExecutor: func threw unhandled "
                    << typeid(e).name() << " exception: " << e.what();
    } catch (...) {
      RDDLOG(ERROR) << "ThreadPoolExecutor: func threw unhandled non-exception "
                       "object";
    }
    task.stats_.runTime = timestampNow() - startTime;
  }
  thread->idle = true;
  thread->lastActiveTime = timestampNow();
  auto lockedCallbacks = thread->taskStatsCallbacks->callbackList.rlock();
  *thread->taskStatsCallbacks->inCallback = true;
  SCOPE_EXIT {
    *thread->taskStatsCallbacks->inCallback = false;
  };
  try {
    for (auto& callback : *lockedCallbacks) {
      callback(task.stats_);
    }
  } catch (const std::exception& e) {
    RDDLOG(ERROR) << "ThreadPoolExecutor: task stats callback threw unhandled "
                  << typeid(e).name() << " exception: " << e.what();
  } catch (...) {
    RDDLOG(ERROR) << "ThreadPoolExecutor: task stats callback threw "
                     "unhandled non-exception object";
  }
}

size_t ThreadPoolExecutor::numThreads() {
  RWSpinLock::ReadHolder r{&threadListLock_};
  return threadList_.get().size();
}

void ThreadPoolExecutor::setNumThreads(size_t n) {
  size_t numThreadsToJoin = 0;
  {
    RWSpinLock::WriteHolder w{&threadListLock_};
    const auto current = threadList_.get().size();
    if (n > current) {
      addThreads(n - current);
    } else if (n < current) {
      numThreadsToJoin = current - n;
      removeThreads(numThreadsToJoin, true);
    }
  }
  joinStoppedThreads(numThreadsToJoin);
  RDDCHECK_EQ(n, threadList_.get().size());
  RDDCHECK_EQ(0, stoppedThreads_.size());
}

// threadListLock_ is writelocked
void ThreadPoolExecutor::addThreads(size_t n) {
  std::vector<ThreadPtr> newThreads;
  for (size_t i = 0; i < n; i++) {
    newThreads.push_back(makeThread());
  }
  for (auto& thread : newThreads) {
    // TODO need a notion of failing to create the thread
    // and then handling for that case
    thread->handle = threadFactory_->newThread(
        std::bind(&ThreadPoolExecutor::threadRun, this, thread));
    threadList_.add(thread);
  }
  for (auto& thread : newThreads) {
    thread->startupBaton.wait();
  }
  for (auto& o : observers_) {
    for (auto& thread : newThreads) {
      o->threadStarted(thread.get());
    }
  }
}

// threadListLock_ is writelocked
void ThreadPoolExecutor::removeThreads(size_t n, bool isJoin) {
  RDDCHECK_LE(n, threadList_.get().size());
  isJoin_ = isJoin;
  stopThreads(n);
}

void ThreadPoolExecutor::joinStoppedThreads(size_t n) {
  for (size_t i = 0; i < n; i++) {
    auto thread = stoppedThreads_.take();
    thread->handle.join();
  }
}

void ThreadPoolExecutor::stop() {
  size_t n = 0;
  {
    RWSpinLock::WriteHolder w{&threadListLock_};
    n = threadList_.get().size();
    removeThreads(n, false);
  }
  joinStoppedThreads(n);
  RDDCHECK_EQ(0, threadList_.get().size());
  RDDCHECK_EQ(0, stoppedThreads_.size());
}

void ThreadPoolExecutor::join() {
  size_t n = 0;
  {
    RWSpinLock::WriteHolder w{&threadListLock_};
    n = threadList_.get().size();
    removeThreads(n, true);
  }
  joinStoppedThreads(n);
  RDDCHECK_EQ(0, threadList_.get().size());
  RDDCHECK_EQ(0, stoppedThreads_.size());
}

ThreadPoolExecutor::PoolStats ThreadPoolExecutor::getPoolStats() {
  const auto now = timestampNow();
  RWSpinLock::ReadHolder r{&threadListLock_};
  ThreadPoolExecutor::PoolStats stats;
  stats.threadCount = threadList_.get().size();
  for (auto thread : threadList_.get()) {
    if (thread->idle) {
      stats.idleThreadCount++;
      const uint64_t idleTime = now - thread->lastActiveTime;
      stats.maxIdleTime = std::max(stats.maxIdleTime, idleTime);
    } else {
      stats.activeThreadCount++;
    }
  }
  stats.pendingTaskCount = getPendingTaskCount();
  stats.totalTaskCount = stats.pendingTaskCount + stats.activeThreadCount;
  return stats;
}

std::atomic<uint64_t> ThreadPoolExecutor::Thread::nextId(0);

void ThreadPoolExecutor::subscribeToTaskStats(TaskStatsCallback cb) {
  if (*taskStatsCallbacks_->inCallback) {
    throw std::runtime_error("cannot subscribe in task stats callback");
  }
  taskStatsCallbacks_->callbackList.wlock()->push_back(std::move(cb));
}

void ThreadPoolExecutor::addObserver(std::shared_ptr<Observer> o) {
  RWSpinLock::ReadHolder r{&threadListLock_};
  observers_.push_back(o);
  for (auto& thread : threadList_.get()) {
    o->threadPreviouslyStarted(thread.get());
  }
}

void ThreadPoolExecutor::removeObserver(std::shared_ptr<Observer> o) {
  RWSpinLock::ReadHolder r{&threadListLock_};
  for (auto& thread : threadList_.get()) {
    o->threadNotYetStopped(thread.get());
  }

  for (auto it = observers_.begin(); it != observers_.end(); it++) {
    if (*it == o) {
      observers_.erase(it);
      return;
    }
  }
  DCHECK(false);
}

} // namespace rdd
