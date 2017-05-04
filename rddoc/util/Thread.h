/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <atomic>
#include <pthread.h>

namespace rdd {

class ThreadTask {
public:
  ThreadTask() : id_(incr_id_++) {}
  virtual ~ThreadTask() {}

  virtual void run() = 0;

  uint64_t id() const { return id_; }
  bool success() const { return success_; }
  uint64_t timecost() const { return timecost_; }

protected:
  bool success_{false};
  uint64_t timecost_{0};

private:
  static std::atomic<uint64_t> incr_id_;
  uint64_t id_;
};

class ThreadBase {
public:
  ThreadBase() {}

  virtual ~ThreadBase() {
    assert(!running_);
  }

  void start(bool detach = false) {
    assert(!running_);
    if (pthread_create(&tid_, nullptr, routine, this)) {
      abort();
    }
    running_ = true;
    if (detach) {
      pthread_detach(tid_);
    }
  }

  void join() {
    assert(running_);
    pthread_join(tid_, nullptr);
    running_ = false;
  }

protected:
  virtual void run() = 0;

private:
  static void* routine(void* ptr) {
    ((ThreadBase*)ptr)->run();
    return nullptr;
  }

  bool running_{false};
  pthread_t tid_;
};

}

