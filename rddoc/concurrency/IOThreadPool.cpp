/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/concurrency/IOThreadPool.h"

namespace rdd {

IOThreadPool::IOThreadPool(
    size_t numThreads,
    std::shared_ptr<ThreadFactory> threadFactory,
    bool bindCpu,
    EventLoopManager* manager)
  : ThreadPool(numThreads, std::move(threadFactory), bindCpu),
    nextThread_(0),
    eventLoopManager_(manager) {
  addThreads(numThreads);
  RDDCHECK(threads_.size() == numThreads);
}

IOThreadPool::~IOThreadPool() {
  stop();
}

void IOThreadPool::add(VoidFunc func) {
  add(std::move(func), 0);
}

void IOThreadPool::add(VoidFunc func,
                       uint64_t expiration,
                       VoidFunc expireCallback) {
  RWSpinLock::ReadHolder{&threadsLock_};
  if (threads_.empty()) {
    throw std::runtime_error("No threads available");
  }
  auto ioThread = pickThread();

  auto task = Task(std::move(func), expiration, std::move(expireCallback));
  auto wrappedVoidFunc = [ ioThread, task ]() mutable {
    runTask(ioThread, std::move(task));
    ioThread->pendingTasks--;
  };

  ioThread->pendingTasks++;
  ioThread->eventLoop->addCallback(std::move(wrappedVoidFunc));
}

EventLoop* IOThreadPool::getEventLoop() {
  return pickThread()->eventLoop;
}

EventLoop* IOThreadPool::getEventLoop(Thread* thread) {
  auto ioThread = dynamic_cast<IOThread*>(thread);
  return ioThread ? ioThread->eventLoop : nullptr;
}

EventLoopManager* IOThreadPool::getEventLoopManager() {
  return eventLoopManager_;
}

std::shared_ptr<IOThreadPool::IOThread> IOThreadPool::pickThread() {
  if (*thisThread_) {
    return *thisThread_;
  }
  auto thread = threads_[nextThread_++ % threads_.size()];
  return std::static_pointer_cast<IOThread>(thread);
}

void IOThreadPool::threadRun(ThreadPtr thread) {
  if (thread->bindCpu) {
    setCpuAffinity(thread->id);
  }
  auto ioThread = std::static_pointer_cast<IOThread>(thread);
  ioThread->eventLoop = eventLoopManager_->getEventLoop();
  thisThread_.reset(new std::shared_ptr<IOThread>(ioThread));

  thread->startupBaton.post();
  while (ioThread->shouldRun) {
    ioThread->eventLoop->loop();
  }
  if (isJoin_) {
    while (ioThread->pendingTasks > 0) {
      ioThread->eventLoop->loopOnce();
    }
  }
  stoppedThreads_.add(ioThread);

  std::lock_guard<std::mutex> guard(ioThread->eventLoopShutdownLock_);
  ioThread->eventLoop = nullptr;
}

void IOThreadPool::stopThreads(size_t n) {
  for (size_t i = 0; i < n; i++) {
    auto ioThread = std::static_pointer_cast<IOThread>(threads_[i]);
    for (auto& o : observers_) {
      o->threadStopped(ioThread.get());
    }
    ioThread->shouldRun = false;
    std::lock_guard<std::mutex> guard(ioThread->eventLoopShutdownLock_);
    if (ioThread->eventLoop) {
      ioThread->eventLoop->stop();
    }
  }
}

size_t IOThreadPool::getPendingTaskCount() {
  size_t count = 0;
  for (const auto& thread : threads_) {
    auto ioThread = std::static_pointer_cast<IOThread>(thread);
    size_t pendingTasks = ioThread->pendingTasks;
    if (pendingTasks > 0 && !ioThread->idle) {
      pendingTasks--;
    }
    count += pendingTasks;
  }
  return count;
}

} // namespace rdd
