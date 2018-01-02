/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/concurrency/ThreadedExecutor.h"

#include <chrono>

#include "raster/util/Logging.h"

namespace rdd {

ThreadedExecutor::ThreadedExecutor(std::shared_ptr<ThreadFactory> threadFactory)
    : threadFactory_(std::move(threadFactory)) {
  controlt_ = std::thread([this] { control(); });
}

ThreadedExecutor::~ThreadedExecutor() {
  stopping_.store(true, std::memory_order_release);
  controlw_.notify_one();
  controlt_.join();
  RDDCHECK(running_.empty());
  RDDCHECK(finished_.empty());
}

void ThreadedExecutor::add(VoidFunc func) {
  RDDCHECK(!stopping_.load(std::memory_order_acquire));
  {
    std::unique_lock<std::mutex> lock(enqueuedm_);
    enqueued_.push_back(std::move(func));
  }
  controlw_.notify_one();
}

std::shared_ptr<ThreadFactory> ThreadedExecutor::newDefaultThreadFactory() {
  return std::make_shared<ThreadFactory>("Threaded");
}

void ThreadedExecutor::control() {
  setCurrentThreadName("ThreadedCtrl");
  constexpr auto kMaxWait = std::chrono::seconds(10);
  auto looping = true;
  while (looping) {
    controlw_.wait(kMaxWait);
    looping = controlPerformAll();
  }
}

void ThreadedExecutor::work(VoidFunc& func) {
  func();
  auto id = std::this_thread::get_id();
  {
    std::unique_lock<std::mutex> lock(finishedm_);
    finished_.push_back(id);
  }
  controlw_.notify_one();
}

void ThreadedExecutor::controlJoinFinishedThreads() {
  std::deque<std::thread::id> finishedt;
  {
    std::unique_lock<std::mutex> lock(finishedm_);
    std::swap(finishedt, finished_);
  }
  for (auto id : finishedt) {
    running_[id].join();
    running_.erase(id);
  }
}

void ThreadedExecutor::controlLaunchEnqueuedTasks() {
  std::deque<VoidFunc> enqueuedt;
  {
    std::unique_lock<std::mutex> lock(enqueuedm_);
    std::swap(enqueuedt, enqueued_);
  }
  for (auto& f : enqueuedt) {
    auto th = threadFactory_->newThread([&]() mutable { work(f); });
    auto id = th.get_id();
    running_[id] = std::move(th);
  }
}

bool ThreadedExecutor::controlPerformAll() {
  auto stopping = stopping_.load(std::memory_order_acquire);
  controlJoinFinishedThreads();
  controlLaunchEnqueuedTasks();
  return !stopping || !running_.empty();
}
} // namespace rdd
