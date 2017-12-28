/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "raster/concurrency/Executor.h"
#include "raster/concurrency/ThreadFactory.h"
#include "raster/thread/Waiter.h"

namespace rdd {

/**
 *  ThreadedExecutor - An executor for blocking tasks.
 *
 *  This executor runs each task in its own thread. It works well for tasks
 *  which mostly sleep, but works poorly for tasks which mostly compute.
 *
 *  For each task given to the executor with `add`, the executor spawns a new
 *  thread for that task, runs the task in that thread, and joins the thread
 *  after the task has completed.
 *
 *  Spawning and joining task threads are done in the executor's internal
 *  control thread. Calls to `add` put the tasks to be run into a queue, where
 *  the control thread will find them.
 *
 *  There is currently no limitation on, or throttling of, concurrency.
 *
 *  This executor is not currently optimized for performance. For example, it
 *  makes no attempt to re-use task threads. Rather, it exists primarily to
 *  offload sleep-heavy tasks from the CPU executor, where they might otherwise
 *  be run.
 */
class ThreadedExecutor : public virtual Executor {
 public:
  explicit ThreadedExecutor(
      std::shared_ptr<ThreadFactory> threadFactory = newDefaultThreadFactory());
  virtual ~ThreadedExecutor();

  ThreadedExecutor(ThreadedExecutor const&) = delete;
  ThreadedExecutor& operator=(ThreadedExecutor const&) = delete;

  ThreadedExecutor(ThreadedExecutor&&) = delete;
  ThreadedExecutor& operator=(ThreadedExecutor&&) = delete;

  void add(VoidFunc func) override;

 private:
  static std::shared_ptr<ThreadFactory> newDefaultThreadFactory();

  void control();
  bool controlPerformAll();
  void controlJoinFinishedThreads();
  void controlLaunchEnqueuedTasks();

  void work(VoidFunc& func);

  std::shared_ptr<ThreadFactory> threadFactory_;

  std::atomic<bool> stopping_{false};

  Waiter controlw_;
  std::thread controlt_;

  std::mutex enqueuedm_;
  std::deque<VoidFunc> enqueued_;

  //  Accessed only by the control thread, so no synchronization.
  std::map<std::thread::id, std::thread> running_;

  std::mutex finishedm_;
  std::deque<std::thread::id> finished_;
};
} // namespace rdd
