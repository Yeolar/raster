/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstdlib>
#include <utility>
#include <ucontext.h>
#include "rddoc/util/Logging.h"
#include "rddoc/util/noncopyable.h"

namespace rdd {

class Context : noncopyable {
public:
  Context() {}
  ~Context() {
    if (ptr_) {
      freeStack();
    }
  }

  template <class F, class... Ts>
  void make(int stackSize, F&& func, Ts&&... args) {
    allocStack(stackSize);
    getcontext(&ctx_);
    ctx_.uc_stack.ss_sp = ptr_;
    ctx_.uc_stack.ss_size = size_;
    makecontext(&ctx_,
                (void (*)(void))func,
                sizeof...(Ts),
                std::forward<Ts>(args)...);
  }

  ucontext_t* ctx() { return &ctx_; }

  void recordStackPosition() {
    int dummy;
    used_ = (char*)ptr_ + size_ - (char*)&dummy;
    if (used_ >= size_) {
      RDDLOG(FATAL) << "context stack usage: " << used_ << "/" << size_;
    }
    RDDLOG(V5) << "context stack usage: " << used_ << "/" << size_;
  }

private:
  void allocStack(int stackSize) {
    ptr_ = malloc(stackSize);
    size_ = stackSize;
  }
  void freeStack() {
    free(ptr_);
  }

  ucontext_t ctx_;
  void* ptr_{nullptr};
  int size_{0};
  int used_{0};
};

inline void swapContext(Context* oldCtx, Context* newCtx) {
  swapcontext(oldCtx->ctx(), newCtx->ctx());
}

}

