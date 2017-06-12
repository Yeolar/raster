/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/concurrency/TaskThreadPool.h"

namespace rdd {

void TaskThreadPool::run() {
  while (true) {
    Task* task = nullptr;
    while (!(task = getTask())) {
      task_waiter_.wait();
    }
    RDDLOG(V2) << "get task("
      << (void*)task << ", " << task->statusLabel()
      << "), unhandle count: " << waitingTaskCount();

    TaskThread* thread = nullptr;
    while (!(thread = getFreeTaskThread())) {
      thread_waiter_.wait();
    }
    RDDLOG(V2) << "get thread(" << thread->id()
      << "), free count: " << thread_free_count_;

    int status = task->status();
    // re-add task should not be released
    if (status == Task::INIT) {
      if (task->isTimeout() || task->isConnectionClosed()) {
        RDDLOG(WARN) << "remove timeout/unconnect task("
          << (void*)task << ")";
        task->close();
        delete task;
        continue;
      }
    }
    assert((status == Task::INIT) ||
           (status == Task::RUNABLE) ||
           (status == Task::BLOCK));
    task->setStatus(Task::RUNABLE);

    if (!thread->setTask(task)) {
      addTask(task);
    }
  }
}

TaskThread* TaskThreadPool::getFreeTaskThread() {
  LockGuard guard(thread_lock_);
  if (!options_.bindcpu) {
    recycle();
  }
  for (auto& t : threads_) {
    if (t->isFree()) {
      return t.get();
    }
  }
  if (!options_.bindcpu) {
    size_t n = threads_.size();
    if (n < options_.thread_count_limit) {
      addTaskThread(n, false);
      return threads_.back().get();
    }
  }
  RDDLOG(WARN) << "cannot find a free thread: "
    << thread_free_count_ << "/" << threads_.size();
  return nullptr;
}

void TaskThreadPool::addTaskThread(int index, bool bindcpu) {
  auto thread = std::make_shared<TaskThread>(this, index, bindcpu);
  thread->start();
  threads_.push_back(thread);
  thread_free_count_++;
}

void TaskThreadPool::createTaskThreads() {
  if (created_) {
    return;
  }
  created_ = true;
  if (options_.bindcpu &&
      options_.thread_count > (size_t)std::max(getCpuNum() - 2, 0)) {
    options_.bindcpu = false;
    RDDLOG(ERROR) << "too many threads to bind to CPU, canceled";
  }
  for (size_t i = 0; i < options_.thread_count; ++i) {
    addTaskThread(i, options_.bindcpu);
  }
}

void TaskThreadPool::recycle() {
  if (threads_.size() > options_.thread_count && timer_.isExpired()) {
    size_t n = std::min(threads_.size() - options_.thread_count,
                        thread_free_count_ / 2);
    for (auto it = threads_.begin(); it != threads_.end(); ) {
      if (n == 0) {
        break;
      }
      if ((*it)->isFree()) {
        threads_.erase(it++);
        n--;
      } else {
        it++;
      }
    }
  }
}

}

