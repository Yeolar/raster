/*
 * Copyright (c) 2011 The LevelDB Authors. All rights reserved.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

namespace rdd {

inline void MemoryBarrier() {
  // See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
  // this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
  __asm__ __volatile__("" : : : "memory");
}

class AtomicPtr {
 public:
  AtomicPtr() {}

  explicit AtomicPtr(void* p) : rep_(p) {}

  inline void* NoBarrier_Load() const {
    return rep_;
  }

  inline void NoBarrier_Store(void* v) {
    rep_ = v;
  }

  inline void* Acquire_Load() const {
    void* result = rep_;
    MemoryBarrier();
    return result;
  }

  inline void Release_Store(void* v) {
    MemoryBarrier();
    rep_ = v;
  }

 private:
  void* rep_;
};

} // namespace rdd
