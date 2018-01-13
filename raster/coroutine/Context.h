/*
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

#include <ucontext.h>

#include "raster/util/Function.h"

namespace rdd {

class Context {
 public:
  Context(VoidFunc&& func,
          unsigned char* stackLimit,
          size_t stackSize)
    : func_(std::move(func)) {
    getcontext(&fiberContext_);
    fiberContext_.uc_stack.ss_sp = stackLimit;
    fiberContext_.uc_stack.ss_size = stackSize;
    makecontext(&fiberContext_, (void (*)())fiberFunc, 1, this);
  }

  void activate() {
    swapcontext(&mainContext_, &fiberContext_);
  }

  void deactivate() {
    swapcontext(&fiberContext_, &mainContext_);
  }

 private:
  static void fiberFunc(intptr_t arg) {
    auto ctx = reinterpret_cast<Context*>(arg);
    ctx->func_();
  }

  VoidFunc func_;
  ucontext_t fiberContext_;
  ucontext_t mainContext_;
};

} // namespace rdd
