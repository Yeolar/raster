/*
 * Copyright (C) 2017, Yeolar
 */

/*
 * N.B. You most likely do _not_ want to use MicroSpinLock or any
 * other kind of spinlock.  Consider MicroLock instead.
 *
 * In short, spinlocks in preemptive multi-tasking operating systems
 * have serious problems and fast mutexes like std::mutex are almost
 * certainly the better choice, because letting the OS scheduler put a
 * thread to sleep is better for system responsiveness and throughput
 * than wasting a timeslice repeatedly querying a lock held by a
 * thread that's blocked, and you can't prevent userspace
 * programs blocking.
 *
 * Spinlocks in an operating system kernel make much more sense than
 * they do in userspace.
 */

#pragma once

/*
 * @author Keith Adams <kma@fb.com>
 * @author Jordan DeLong <delong.j@fb.com>
 */

#include <type_traits>
#include <atomic>
#include "rddoc/util/Logging.h"
#include "rddoc/util/noncopyable.h"
#include "rddoc/util/SysUtil.h"

namespace rdd {

namespace detail {

/*
 * A helper object for the contended case. Starts off with eager
 * spinning, and falls back to sleeping for small quantums.
 */
class Sleeper {
  static const uint32_t kMaxActiveSpin = 4000;

  uint32_t spinCount;

public:
  Sleeper() : spinCount(0) {}

  void wait() {
    if (spinCount < kMaxActiveSpin) {
      ++spinCount;
      asm_volatile_pause();
    } else {
      /*
       * Always sleep 0.5ms, assuming this will make the kernel put
       * us down for whatever its minimum timer resolution is (in
       * linux this varies by kernel version from 1ms to 10ms).
       */
      struct timespec ts = { 0, 500000 };
      nanosleep(&ts, nullptr);
    }
  }
};

} // namespace detail

/*
 * A really, *really* small spinlock for fine-grained locking of lots
 * of teeny-tiny data.
 *
 * Zero initializing these is guaranteed to be as good as calling
 * init(), since the free state is guaranteed to be all-bits zero.
 *
 * This class should be kept a POD, so we can used it in other packed
 * structs (gcc does not allow __attribute__((__packed__)) on structs that
 * contain non-POD data).  This means avoid adding a constructor, or
 * making some members private, etc.
 */
struct MicroSpinLock {
  enum { FREE = 0, LOCKED = 1 };
  // lock_ can't be std::atomic<> to preserve POD-ness.
  uint8_t lock_;

  // Initialize this MSL.  It is unnecessary to call this if you
  // zero-initialize the MicroSpinLock.
  void init() {
    payload()->store(FREE);
  }

  bool try_lock() {
    return cas(FREE, LOCKED);
  }

  void lock() {
    detail::Sleeper sleeper;
    do {
      while (payload()->load() != FREE) {
        sleeper.wait();
      }
    } while (!try_lock());
  }

  void unlock() {
    RDDCHECK(payload()->load() == LOCKED);
    payload()->store(FREE, std::memory_order_release);
  }

 private:
  std::atomic<uint8_t>* payload() {
    return reinterpret_cast<std::atomic<uint8_t>*>(&this->lock_);
  }

  bool cas(uint8_t compare, uint8_t newVal) {
    return std::atomic_compare_exchange_strong_explicit(
        payload(), &compare, newVal,
        std::memory_order_acquire,
        std::memory_order_relaxed);
  }
};
static_assert(
    std::is_pod<MicroSpinLock>::value,
    "MicroSpinLock must be kept a POD type.");

class SpinLock {
public:
  SpinLock() {
    lock_.init();
  }

  void lock() const {
    lock_.lock();
  }

  void unlock() const {
    lock_.unlock();
  }

  bool try_lock() const {
    return lock_.try_lock();
  }

private:
  mutable MicroSpinLock lock_;
};

template <class LOCK>
class SpinLockGuardImpl : noncopyable {
public:
  explicit SpinLockGuardImpl(LOCK& lock) : lock_(lock) {
    lock_.lock();
  }

  ~SpinLockGuardImpl() {
    lock_.unlock();
  }

private:
  LOCK& lock_;
};

typedef SpinLockGuardImpl<SpinLock> SpinLockGuard;

} // namespace rdd
