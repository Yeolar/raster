/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <vector>
#include "rddoc/coroutine/TaskThread.h"
#include "rddoc/util/LockedDeq.h"
#include "rddoc/util/LockedSet.h"

namespace rdd {

class TaskThreadPool : noncopyable {
public:
  struct Options {
    size_t thread_count{16};
    size_t thread_count_limit{0};
    bool bindcpu{false};
    size_t waiting_task_limit{100};
  };

  TaskThreadPool(int pid)
    : pid_(pid), timer_(600000000) {}

  void setOptions(const Options& options) {
    options_ = options;
  }

  static void* routine(void* ptr) {
    ((TaskThreadPool*)ptr)->run();
    return nullptr;
  }

  void start() {
    createTaskThreads();
    createThread(TaskThreadPool::routine, this);
  }

  void run();

  TaskThread* getFreeTaskThread();

  void increaseFreeCount() {
    thread_free_count_++;
    thread_waiter_.signal();
  }
  void decreaseFreeCount() {
    thread_free_count_--;
  }
  size_t freeThreadCount() const {
    return thread_free_count_;
  }

  void addTask(Task* task) {
    task->record(Timestamp(Task::WAIT));
    task_deq_.push(task);
    task_waiter_.signal();
  }
  Task* getTask() {
    return task_deq_.pop();
  }
  size_t waitingTaskCount() const {
    return task_deq_.size();
  }
  bool exceedWaitingTaskLimit() const {
    return waitingTaskCount() >= options_.waiting_task_limit;
  }

private:
  void addTaskThread(int index, bool bindcpu);
  void createTaskThreads();
  void recycle();

  int pid_;
  Options options_;
  std::vector<std::shared_ptr<TaskThread>> threads_;
  Lock thread_lock_;
  std::atomic<size_t> thread_free_count_{0};
  CycleTimer timer_;
  bool created_{false};
  LockedDeq<Task*> task_deq_;
  Waiter task_waiter_;
  Waiter thread_waiter_;
};

}
