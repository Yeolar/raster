/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <unistd.h>
#include "rddoc/coroutine/Task.h"
#include "rddoc/util/SysUtil.h"
#include "rddoc/util/Waiter.h"

namespace rdd {

class TaskThreadPool;

class TaskThread : noncopyable {
public:
  TaskThread(TaskThreadPool* pool, int index, bool bindcpu = false)
    : pool_(pool), index_(index), bindcpu_(bindcpu) {}

  ~TaskThread() {
    stop();
  }

  static void* routine(void* ptr) {
    TaskThread* t = (TaskThread*)ptr;
    if (t->bindcpu_) {
      setCpuAffinity(t->index_);
    }
    t->run();
    return nullptr;
  }

  void start() {
    if (!running_) {
      pthread_create(&tid_, 0, TaskThread::routine, this);
    }
  }

  void stop() {
    if (running_) {
      running_ = false;
      waiter_.signal();
      pthread_join(tid_, nullptr);
    }
  }

  void run();

  bool setTask(Task* task);

  pid_t id() const { return pid_; }
  bool isFree() const { return free_; }

private:
  TaskThreadPool* pool_;
  int index_;
  bool bindcpu_;
  pid_t pid_{0};
  pthread_t tid_;
  std::atomic<bool> running_{false};
  std::atomic<bool> free_{true};
  Task* task_{nullptr};
  Waiter waiter_;
};

}

