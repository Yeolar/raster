/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

namespace rdd {

class Worker {
public:
  Worker() {}

  static void* routine(void* ptr) {
    Worker* w = (Worker*)ptr;
    if (w->bindcpu_) {
      setCpuAffinity(w->index_);
    }
    w->run();
    return nullptr;
  }

  void start() {
    if (!running_) {
      pthread_create(&tid_, 0, Worker::routine, this);
    }
  }

  void stop() {
    loop_->stop();
    pthread_join(tid_, nullptr);
  }

  void run() {
    loop_->start();
  }

private:
  EventLoop loop_;
};

}

