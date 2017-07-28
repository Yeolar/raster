/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <ucontext.h>
#include "rddoc/util/Function.h"

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
