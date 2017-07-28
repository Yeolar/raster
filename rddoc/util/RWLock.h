/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <errno.h>
#include <pthread.h>
#include "rddoc/util/noncopyable.h"

namespace rdd {

class RWLock : noncopyable {
public:
  RWLock() {
    pthread_rwlock_init(&lock_, nullptr);
  }
  ~RWLock() {
    pthread_rwlock_destroy(&lock_);
  }

  void rdlock() const {
    pthread_rwlock_rdlock(&lock_);
  }
  void wrlock() const {
    pthread_rwlock_wrlock(&lock_);
  }
  void unlock() const {
    pthread_rwlock_unlock(&lock_);
  }

private:
  mutable pthread_rwlock_t lock_;
};

template <bool write>
class RWLockGuardImpl : noncopyable {
public:
  explicit RWLockGuardImpl(RWLock& lock)
    : lock_(lock) {
    if (write) {
      lock_.wrlock();
    } else {
      lock_.rdlock();
    }
  }
  ~RWLockGuardImpl() {
    lock_.unlock();
  }

private:
  RWLock& lock_;
};

typedef RWLockGuardImpl<false> RLockGuard;
typedef RWLockGuardImpl<true> WLockGuard;

} // namespace rdd
