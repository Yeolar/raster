/*
 * Copyright 2017 Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * N.B. You most likely do _not_ want to use RWSpinLock or any other
 * kind of spinlock.  Use SharedMutex instead.
 *
 * In short, spinlocks in preemptive multi-tasking operating systems
 * have serious problems and fast mutexes like SharedMutex are almost
 * certainly the better choice, because letting the OS scheduler put a
 * thread to sleep is better for system responsiveness and throughput
 * than wasting a timeslice repeatedly querying a lock held by a
 * thread that's blocked, and you can't prevent userspace
 * programs blocking.
 *
 * Spinlocks in an operating system kernel make much more sense than
 * they do in userspace.
 *
 * -------------------------------------------------------------------
 *
 * Read-Write spin lock implementations.
 *
 *  Ref: http://locklessinc.com/articles/locks
 *
 *  Faster than pthread_rwlock and have very low overhead (usually
 *  20-30ns).  They don't use any system mutexes and are very compact
 *  (4/8 bytes), so are suitable for per-instance based locking,
 *  particularly when contention is not expected.
 *
 *  For a spinlock, RWSpinLock is a reasonable choice.  (See the note
 *  about for why a spin lock is frequently a bad idea generally.)
 *  RWSpinLock has minimal overhead, and comparable contention
 *  performance when the number of competing threads is less than or
 *  equal to the number of logical CPUs.  Even as the number of
 *  threads gets larger, RWSpinLock can still be very competitive in
 *  READ, although it is slower on WRITE, and also inherently unfair
 *  to writers.
 *
 * @author Xin Liu <xliux@fb.com>
 */

#pragma once

#include <atomic>
#include <thread>

#include "raster/util/Macro.h"

namespace rdd {

/*
 * A simple, small (4-bytes), but unfair rwlock.  Use it when you want
 * a nice writer and don't expect a lot of write/read contention, or
 * when you need small rwlocks since you are creating a large number
 * of them.
 *
 * Note that the unfairness here is extreme: if the lock is
 * continually accessed for read, writers will never get a chance.  If
 * the lock can be that highly contended this class is probably not an
 * ideal choice anyway.
 *
 * It currently implements most of the Lockable, SharedLockable and
 * UpgradeLockable concepts except the TimedLockable related locking/unlocking
 * interfaces.
 */
class RWSpinLock {
  enum : int32_t { READER = 4, UPGRADED = 2, WRITER = 1 };
 public:
  constexpr RWSpinLock() : bits_(0) {}

  RWSpinLock(RWSpinLock const&) = delete;
  RWSpinLock& operator=(RWSpinLock const&) = delete;

  // Lockable Concept
  void lock() {
    uint_fast32_t count = 0;
    while (!LIKELY(try_lock())) {
      if (++count > 1000) {
        std::this_thread::yield();
      }
    }
  }

  // Writer is responsible for clearing up both the UPGRADED and WRITER bits.
  void unlock() {
    static_assert(READER > WRITER + UPGRADED, "wrong bits!");
    bits_.fetch_and(~(WRITER | UPGRADED), std::memory_order_release);
  }

  // SharedLockable Concept
  void lock_shared() {
    uint_fast32_t count = 0;
    while (!LIKELY(try_lock_shared())) {
      if (++count > 1000) {
        std::this_thread::yield();
      }
    }
  }

  void unlock_shared() {
    bits_.fetch_add(-READER, std::memory_order_release);
  }

  // Downgrade the lock from writer status to reader status.
  void unlock_and_lock_shared() {
    bits_.fetch_add(READER, std::memory_order_acquire);
    unlock();
  }

  // UpgradeLockable Concept
  void lock_upgrade() {
    uint_fast32_t count = 0;
    while (!try_lock_upgrade()) {
      if (++count > 1000) {
        std::this_thread::yield();
      }
    }
  }

  void unlock_upgrade() {
    bits_.fetch_add(-UPGRADED, std::memory_order_acq_rel);
  }

  // unlock upgrade and try to acquire write lock
  void unlock_upgrade_and_lock() {
    int64_t count = 0;
    while (!try_unlock_upgrade_and_lock()) {
      if (++count > 1000) {
        std::this_thread::yield();
      }
    }
  }

  // unlock upgrade and read lock atomically
  void unlock_upgrade_and_lock_shared() {
    bits_.fetch_add(READER - UPGRADED, std::memory_order_acq_rel);
  }

  // write unlock and upgrade lock atomically
  void unlock_and_lock_upgrade() {
    // need to do it in two steps here -- as the UPGRADED bit might be OR-ed at
    // the same time when other threads are trying do try_lock_upgrade().
    bits_.fetch_or(UPGRADED, std::memory_order_acquire);
    bits_.fetch_add(-WRITER, std::memory_order_release);
  }


  // Attempt to acquire writer permission. Return false if we didn't get it.
  bool try_lock() {
    int32_t expect = 0;
    return bits_.compare_exchange_strong(expect, WRITER,
      std::memory_order_acq_rel);
  }

  // Try to get reader permission on the lock. This can fail if we
  // find out someone is a writer or upgrader.
  // Setting the UPGRADED bit would allow a writer-to-be to indicate
  // its intention to write and block any new readers while waiting
  // for existing readers to finish and release their read locks. This
  // helps avoid starving writers (promoted from upgraders).
  bool try_lock_shared() {
    // fetch_add is considerably (100%) faster than compare_exchange,
    // so here we are optimizing for the common (lock success) case.
    int32_t value = bits_.fetch_add(READER, std::memory_order_acquire);
    if (UNLIKELY(value & (WRITER|UPGRADED))) {
      bits_.fetch_add(-READER, std::memory_order_release);
      return false;
    }
    return true;
  }

  // try to unlock upgrade and write lock atomically
  bool try_unlock_upgrade_and_lock() {
    int32_t expect = UPGRADED;
    return bits_.compare_exchange_strong(expect, WRITER,
        std::memory_order_acq_rel);
  }

  // try to acquire an upgradable lock.
  bool try_lock_upgrade() {
    int32_t value = bits_.fetch_or(UPGRADED, std::memory_order_acquire);

    // Note: when failed, we cannot flip the UPGRADED bit back,
    // as in this case there is either another upgrade lock or a write lock.
    // If it's a write lock, the bit will get cleared up when that lock's done
    // with unlock().
    return ((value & (UPGRADED | WRITER)) == 0);
  }

  // mainly for debugging purposes.
  int32_t bits() const { return bits_.load(std::memory_order_acquire); }

  class ReadHolder;
  class UpgradedHolder;
  class WriteHolder;

  class ReadHolder {
   public:
    explicit ReadHolder(RWSpinLock* lock) : lock_(lock) {
      if (lock_) {
        lock_->lock_shared();
      }
    }

    explicit ReadHolder(RWSpinLock& lock) : lock_(&lock) {
      lock_->lock_shared();
    }

    ReadHolder(ReadHolder&& other) noexcept : lock_(other.lock_) {
      other.lock_ = nullptr;
    }

    // down-grade
    explicit ReadHolder(UpgradedHolder&& upgraded) : lock_(upgraded.lock_) {
      upgraded.lock_ = nullptr;
      if (lock_) {
        lock_->unlock_upgrade_and_lock_shared();
      }
    }

    explicit ReadHolder(WriteHolder&& writer) : lock_(writer.lock_) {
      writer.lock_ = nullptr;
      if (lock_) {
        lock_->unlock_and_lock_shared();
      }
    }

    ReadHolder& operator=(ReadHolder&& other) {
      using std::swap;
      swap(lock_, other.lock_);
      return *this;
    }

    ReadHolder(const ReadHolder& other) = delete;
    ReadHolder& operator=(const ReadHolder& other) = delete;

    ~ReadHolder() {
      if (lock_) {
        lock_->unlock_shared();
      }
    }

    void reset(RWSpinLock* lock = nullptr) {
      if (lock == lock_) {
        return;
      }
      if (lock_) {
        lock_->unlock_shared();
      }
      lock_ = lock;
      if (lock_) {
        lock_->lock_shared();
      }
    }

    void swap(ReadHolder* other) {
      std::swap(lock_, other->lock_);
    }

   private:
    friend class UpgradedHolder;
    friend class WriteHolder;
    RWSpinLock* lock_;
  };

  class UpgradedHolder {
   public:
    explicit UpgradedHolder(RWSpinLock* lock) : lock_(lock) {
      if (lock_) {
        lock_->lock_upgrade();
      }
    }

    explicit UpgradedHolder(RWSpinLock& lock) : lock_(&lock) {
      lock_->lock_upgrade();
    }

    explicit UpgradedHolder(WriteHolder&& writer) {
      lock_ = writer.lock_;
      writer.lock_ = nullptr;
      if (lock_) {
        lock_->unlock_and_lock_upgrade();
      }
    }

    UpgradedHolder(UpgradedHolder&& other) noexcept : lock_(other.lock_) {
      other.lock_ = nullptr;
    }

    UpgradedHolder& operator =(UpgradedHolder&& other) {
      using std::swap;
      swap(lock_, other.lock_);
      return *this;
    }

    UpgradedHolder(const UpgradedHolder& other) = delete;
    UpgradedHolder& operator =(const UpgradedHolder& other) = delete;

    ~UpgradedHolder() {
      if (lock_) {
        lock_->unlock_upgrade();
      }
    }

    void reset(RWSpinLock* lock = nullptr) {
      if (lock == lock_) {
        return;
      }
      if (lock_) {
        lock_->unlock_upgrade();
      }
      lock_ = lock;
      if (lock_) {
        lock_->lock_upgrade();
      }
    }

    void swap(UpgradedHolder* other) {
      using std::swap;
      swap(lock_, other->lock_);
    }

   private:
    friend class WriteHolder;
    friend class ReadHolder;
    RWSpinLock* lock_;
  };

  class WriteHolder {
   public:
    explicit WriteHolder(RWSpinLock* lock) : lock_(lock) {
      if (lock_) {
        lock_->lock();
      }
    }

    explicit WriteHolder(RWSpinLock& lock) : lock_(&lock) {
      lock_->lock();
    }

    // promoted from an upgrade lock holder
    explicit WriteHolder(UpgradedHolder&& upgraded) {
      lock_ = upgraded.lock_;
      upgraded.lock_ = nullptr;
      if (lock_) {
        lock_->unlock_upgrade_and_lock();
      }
    }

    WriteHolder(WriteHolder&& other) noexcept : lock_(other.lock_) {
      other.lock_ = nullptr;
    }

    WriteHolder& operator =(WriteHolder&& other) {
      using std::swap;
      swap(lock_, other.lock_);
      return *this;
    }

    WriteHolder(const WriteHolder& other) = delete;
    WriteHolder& operator =(const WriteHolder& other) = delete;

    ~WriteHolder() {
      if (lock_) {
        lock_->unlock();
      }
    }

    void reset(RWSpinLock* lock = nullptr) {
      if (lock == lock_) {
        return;
      }
      if (lock_) {
        lock_->unlock();
      }
      lock_ = lock;
      if (lock_) {
        lock_->lock();
      }
    }

    void swap(WriteHolder* other) {
      using std::swap;
      swap(lock_, other->lock_);
    }

   private:
    friend class ReadHolder;
    friend class UpgradedHolder;
    RWSpinLock* lock_;
  };

 private:
  std::atomic<int32_t> bits_;
};

} // namespace rdd
