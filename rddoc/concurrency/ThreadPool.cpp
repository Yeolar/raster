/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/concurrency/ThreadPool.h"

namespace rdd {

std::atomic<uint64_t> ThreadPool::Thread::nextId(0);

size_t ThreadPool::numThreads() {
  RWSpinLock::ReadHolder{&threadsLock_};
  return threads_.size();
}

void ThreadPool::setNumThreads(size_t n) {
  RWSpinLock::WriteHolder{&threadsLock_};
  const auto current = threads_.size();
  if (n > current ) {
    addThreads(n - current);
  } else if (n < current) {
    removeThreads(current - n, true);
  }
  RDDCHECK(threads_.size() == n);
}

void ThreadPool::stop() {
  RWSpinLock::WriteHolder{&threadsLock_};
  removeThreads(threads_.size(), false);
  RDDCHECK(threads_.size() == 0);
}

void ThreadPool::join() {
  RWSpinLock::WriteHolder{&threadsLock_};
  removeThreads(threads_.size(), true);
  RDDCHECK(threads_.size() == 0);
}

ThreadPool::Stats ThreadPool::getStats() {
  RWSpinLock::ReadHolder{&threadsLock_};
  ThreadPool::Stats stats;
  stats.threadCount = threads_.size();
  for (auto thread : threads_) {
    if (thread->idle) {
      stats.idleThreadCount++;
    } else {
      stats.activeThreadCount++;
    }
  }
  stats.pendingTaskCount = getPendingTaskCount();
  stats.totalTaskCount = stats.pendingTaskCount + stats.activeThreadCount;
  return stats;
}

void ThreadPool::addObserver(std::shared_ptr<Observer> o) {
  RWSpinLock::ReadHolder{&threadsLock_};
  observers_.push_back(o);
  for (auto& thread : threads_) {
    o->threadPreviouslyStarted(thread.get());
  }
}

void ThreadPool::removeObserver(std::shared_ptr<Observer> o) {
  RWSpinLock::ReadHolder{&threadsLock_};
  for (auto& thread : threads_) {
    o->threadNotYetStopped(thread.get());
  }
  for (auto it = observers_.begin(); it != observers_.end(); it++) {
    if (*it == o) {
      observers_.erase(it);
      return;
    }
  }
}

void ThreadPool::addThreads(size_t n) {
  std::vector<ThreadPtr> newThreads;
  for (size_t i = 0; i < n; i++) {
    newThreads.push_back(makeThread());
  }
  for (auto& thread : newThreads) {
    thread->handle = threadFactory_->createThread(
        std::bind(&ThreadPool::threadRun, this, thread));
    threads_.push_back(thread);
  }
  std::sort(threads_.begin(), threads_.end());
  for (auto& thread : newThreads) {
    thread->startupBaton.wait();
  }
  for (auto& o : observers_) {
    for (auto& thread : newThreads) {
      o->threadStarted(thread.get());
    }
  }
}

void ThreadPool::removeThreads(size_t n, bool isJoin) {
  RDDCHECK(stoppedThreads_.size() == 0);
  isJoin_ = isJoin;
  stopThreads(n);
  for (size_t i = 0; i < n; i++) {
    auto thread = stoppedThreads_.take();
    thread->handle.join();
    auto it = std::equal_range(threads_.begin(), threads_.end(), thread);
    RDDCHECK(it.first != threads_.end());
    RDDCHECK(std::next(it.first) == it.second);
    threads_.erase(it.first);
  }
  RDDCHECK(stoppedThreads_.size() == 0);
}

void ThreadPool::runTask(const ThreadPtr& thread, Task&& task) {
  thread->idle = false;
  uint64_t startTime = timestampNow();
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
      RDDLOG(ERROR) << "ThreadPool: func threw unhandled "
        << typeid(e).name() << " exception: " << e.what();
    } catch (...) {
      RDDLOG(ERROR) << "ThreadPool: func threw unhandled "
        << "non-exception object";
    }
    task.stats_.runTime = timestampNow() - startTime;
  }
  thread->idle = true;
  thread->taskStatsSubject->on(std::move(task.stats_));
}

}
