/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <vector>
#include "rddoc/util/LockedDeq.h"
#include "rddoc/util/LockedMap.h"
#include "rddoc/util/Thread.h"

namespace rdd {

template <class Thread>
class ThreadPoolBase {
public:
  ThreadPoolBase(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      threads_.push_back(new Thread(this));
    }
  }
  virtual ~ThreadPoolBase() {
    finish();
  }

  void addTask(ThreadTask* t) {
    task_deq_.push(t);
  }

  ThreadTask* getTask() {
    return task_deq_.pop();
  }

  void finishTask(ThreadTask* t) {
    if (t->success()) {
      ++success_;
      task_costs_.insert(t->id(), t->timecost());
    }
    else {
      ++fail_;
    }
  }

  void start() {
    for (auto& p : threads_) {
      p->start();
    }
  }

  void finish() {
    for (auto& p : threads_) {
      p->join();
      delete p;
    }
    threads_.clear();
  }

  size_t waitingCount() const { return task_deq_.size(); }
  size_t successCount() const { return success_; }
  size_t failCount() const { return fail_; }

  const LockedMap<uint64_t, uint64_t>& taskCosts() {
    return task_costs_;
  }

private:
  std::vector<Thread*> threads_;
  LockedDeq<ThreadTask*> task_deq_;
  std::atomic<size_t> success_{0};
  std::atomic<size_t> fail_{0};
  LockedMap<uint64_t, uint64_t> task_costs_;
};

}

