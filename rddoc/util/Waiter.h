/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <pthread.h>
#include "rddoc/util/noncopyable.h"

namespace rdd {

class Waiter : noncopyable {
public:
  Waiter() : signal_(false) {
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&cond_, nullptr);
  }
  ~Waiter() {
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&cond_);
  }

  void wait() const {
    pthread_mutex_lock(&lock_);
    while (!signal_) {
      pthread_cond_wait(&cond_, &lock_);
    }
    signal_ = false;
    pthread_mutex_unlock(&lock_);
  }
  void signal() const {
    pthread_mutex_lock(&lock_);
    signal_ = true;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&lock_);
  }
  void broadcast() const {
    pthread_mutex_lock(&lock_);
    signal_ = true;
    pthread_cond_broadcast(&cond_);
    pthread_mutex_unlock(&lock_);
  }

private:
  mutable pthread_mutex_t lock_;
  mutable pthread_cond_t cond_;
  mutable bool signal_;
};

}

