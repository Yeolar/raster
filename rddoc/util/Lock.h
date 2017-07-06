/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <errno.h>
#include <pthread.h>
#include "rddoc/util/noncopyable.h"

namespace rdd {

class SpinLock : noncopyable {
public:
  SpinLock() {
    pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
  }
  ~SpinLock() {
    pthread_spin_destroy(&lock_);
  }

  void lock() const {
    pthread_spin_lock(&lock_);
  }
  void unlock() const {
    pthread_spin_unlock(&lock_);
  }
  bool trylock() const {
    int rc = pthread_spin_trylock(&lock_);
    if (rc == 0) {
      return true;
    } else if (rc == EBUSY) {
      return false;
    }
    return false;   // error
  }

private:
  mutable pthread_spinlock_t lock_;
};

template <class T>
class LockGuardImpl : noncopyable {
public:
  explicit LockGuardImpl(T& lock, bool trylock = false)
    : lock_(lock), locked_(true) {
    if (trylock) {
      locked_ = lock_.trylock();
    } else {
      lock_.lock();
    }
  }
  ~LockGuardImpl() {
    if (locked_) {
      lock_.unlock();
    }
  }

private:
  T& lock_;
  bool locked_;
};

typedef LockGuardImpl<SpinLock> SpinLockGuard;

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

}

