/*
 * Copyright (C) 2017, Yeolar
 */

#include <assert.h>
#include "rddoc/coroutine/TaskThread.h"
#include "rddoc/coroutine/TaskThreadPool.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

void TaskThread::run() {
  Context thread_ctx;
  pid_ = localThreadId();
  running_ = true;

  while (running_) {
    while (running_ && free_) {
      waiter_.wait();
    }
    if (task_) {
      ThreadTask::update(task_);
      task_->setThreadContext(&thread_ctx);

      RDDLOG(V2) << "run task("
        << (void*)task_ << ", " << task_->statusLabel() << ")";

      uint64_t t0 = timestampNow();
      task_->execute();
      uint64_t tp = timePassed(t0);

      RDDMON_AVG("taskcost", tp / 1000);
      RDDMON_MAX("taskcost.max", tp / 1000);

      RDDLOG(V2) << "finish task("
        << (void*)task_ << ", " << task_->statusLabel() << ")";

      switch (task_->status()) {
        case Task::RUNABLE:
          pool_->addTask(task_);
          break;
        case Task::BLOCK:
          task_->sweepBlockedCallback();
          break;
        case Task::EXIT:
        default:
          delete task_;
          break;
      }
      task_ = nullptr;
    }
    free_ = true;
    pool_->increaseFreeCount();
  }
}

bool TaskThread::setTask(Task* task) {
  if (!free_) {
    return false;
  }
  task_ = task;
  free_ = false;
  pool_->decreaseFreeCount();
  waiter_.signal();
  return true;
}

}

