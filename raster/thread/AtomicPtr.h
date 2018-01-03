/*
 * Copyright (c) 2011 The LevelDB Authors. All rights reserved.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/util/Asm.h"

namespace rdd {

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
    asm_volatile_memory();
    return result;
  }

  inline void Release_Store(void* v) {
    asm_volatile_memory();
    rep_ = v;
  }

 private:
  void* rep_;
};

} // namespace rdd
