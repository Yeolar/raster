/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <utility>
#include <sched.h>

#include "rddoc/util/Macro.h"
#include "rddoc/util/noncopyable.h"

namespace rdd {

class RWSpinLock : noncopyable {
  enum : int32_t { READER = 2, WRITER = 1 };

public:
  RWSpinLock() : bits_(0) {}

  void lock() {
    int count = 0;
    while (!LIKELY(try_lock())) {
      if (++count > 1000)
        sched_yield();
    }
  }

  void lock_shared() {
    int count = 0;
    while (!LIKELY(try_lock_shared())) {
      if (++count > 1000)
        sched_yield();
    }
  }

  void unlock() {
    bits_.fetch_add(-WRITER, std::memory_order_release);
  }

  void unlock_shared() {
    bits_.fetch_add(-READER, std::memory_order_release);
  }

  bool try_lock() {
    int32_t expect = 0;
    return bits_.compare_exchange_strong(expect, WRITER,
                                         std::memory_order_acq_rel);
  }

  bool try_lock_shared() {
    int32_t value = bits_.fetch_add(READER, std::memory_order_acquire);
    if (UNLIKELY(value & WRITER)) {
      bits_.fetch_add(-READER, std::memory_order_release);
      return false;
    }
    return true;
  }

  class ReadHolder {
  public:
    explicit ReadHolder(RWSpinLock* lock = nullptr) : lock_(lock) {
      if (lock_)
        lock_->lock_shared();
    }
    explicit ReadHolder(RWSpinLock& lock) : lock_(&lock) {
      lock_->lock_shared();
    }
    ReadHolder(ReadHolder&& other) : lock_(other.lock_) {
      other.lock_ = nullptr;
    }

    ReadHolder& operator=(ReadHolder&& other) {
      std::swap(lock_, other.lock_);
      return *this;
    }

    ~ReadHolder() {
      if (lock_)
        lock_->unlock_shared();
    }

    void reset(RWSpinLock* lock = nullptr) {
      if (lock == lock_)
        return;
      if (lock_)
        lock_->unlock_shared();
      lock_ = lock;
      if (lock_)
        lock_->lock_shared();
    }

    void swap(ReadHolder* other) { std::swap(lock_, other->lock_); }

  private:
    ReadHolder(const ReadHolder& other);
    ReadHolder& operator=(const ReadHolder& other);

    RWSpinLock* lock_;
  };

  class WriteHolder {
  public:
    explicit WriteHolder(RWSpinLock* lock = nullptr) : lock_(lock) {
      if (lock_)
        lock_->lock();
    }
    explicit WriteHolder(RWSpinLock& lock) : lock_(&lock) {
      lock_->lock();
    }
    WriteHolder(WriteHolder&& other) : lock_(other.lock_) {
      other.lock_ = nullptr;
    }

    WriteHolder& operator=(WriteHolder&& other) {
      std::swap(lock_, other.lock_);
      return *this;
    }

    ~WriteHolder() {
      if (lock_)
        lock_->unlock();
    }

    void reset(RWSpinLock* lock = nullptr) {
      if (lock == lock_)
        return;
      if (lock_)
        lock_->unlock();
      lock_ = lock;
      if (lock_)
        lock_->lock();
    }

    void swap(WriteHolder* other) { std::swap(lock_, other->lock_); }

  private:
    WriteHolder(const WriteHolder& other);
    WriteHolder& operator=(const WriteHolder& other);

    RWSpinLock* lock_;
  };

private:
  std::atomic<int32_t> bits_;
};

} // namespace rdd
