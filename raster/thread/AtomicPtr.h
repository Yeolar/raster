/*
 * Copyright (c) 2011 The LevelDB Authors.
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
