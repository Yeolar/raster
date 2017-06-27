/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <list>
#include "rddoc/coroutine/Context.h"
#include "rddoc/util/Function.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/String.h"
#include "rddoc/util/ThreadUtil.h"

#define RDD_FCLOG(severity, t) \
  RDDLOG(severity) << "fiber(" << (void*)(t) << ", " \
    << (t)->type() << ", " \
    << (t)->timestampStr() << ") "

namespace rdd {

class Fiber : noncopyable {
public:
  enum {
    INIT,     // I
    RUNABLE,  // A
    RUNNING,  // R
    BLOCK,    // B
    EXIT,     // Q
  };

  static size_t count() { return count_; }

  Fiber(int stackSize)
    : stackSize_(stackSize)
    , qstart_(timestampNow()) {
    timestamps_.emplace_back(Timestamp(status_));
    ++count_;
  }
  virtual ~Fiber() {
    --count_;
  }

  template <class F, class... Ts>
  void set(F&& func, Ts&&... args) {
    context_.make(stackSize_, func, std::forward<Ts>(args)...);
    setStatus(RUNABLE);
  }

  int status() const { return status_; }

  void setStatus(int status) {
    status_ = status;
    record(Timestamp(status_));
  }
  char statusLabel() const {
    return "IARBQW"[status_];
  }

  void setBackContext(Context* context) {
    backContext_ = context;
  }

  template <class T>
  T* data() const { return reinterpret_cast<T*>(data_); }
  template <class T>
  void setData(T* ptr) { data_ = ptr; }

  bool isTimeout() {
    if (qtimeout_ > 0) {
      uint64_t interval = timePassed(qstart_);
      if (interval > qtimeout_) {
        RDDLOG(WARN) << "fiber (" << (void*)this
          << ") is timeout(us): " << interval << ">" << qtimeout_;
        return true;
      }
    }
    return false;
  }

  void execute() {
    assert(backContext_ != nullptr);
    assert(status_ == RUNABLE);
    setStatus(RUNNING);
    swapContext(backContext_, &context_);
  }
  void yield(int status) {
    assert(backContext_ != nullptr);
    assert(status_ == RUNNING);
    setStatus(status);
    context_.recordStackPosition();
    swapContext(&context_, backContext_);
  }

  void addBlockedCallback(const VoidFunc& callback) {
    SpinLockGuard guard(blockedLock_);
    blockedCallback_.emplace_back(callback);
  }
  void sweepBlockedCallback() {
    SpinLockGuard guard(blockedLock_);
    for (auto& callback : blockedCallback_) {
      callback();
    }
    blockedCallback_.clear();
  }

  uint64_t starttime() const {
    return timestamps_.empty() ? 0 : timestamps_.front().stamp;
  }
  void record(Timestamp timestamp) {
    if (!timestamps_.empty()) {
      timestamp.stamp -= starttime();
    }
    timestamps_.emplace_back(timestamp);
  }
  uint64_t cost() const {
    return timestamps_.empty() ? 0 : timePassed(starttime());
  }

  std::string timestampStr() const {
    return join("-", timestamps_);
  }

private:
  static std::atomic<size_t> count_;

  int status_{INIT};
  int stackSize_;
  Context context_;
  Context* backContext_{nullptr};
  void* data_{nullptr};

  uint64_t qstart_;           // deq start
  uint64_t qtimeout_{300000}; // deq timeout
  std::vector<Timestamp> timestamps_;

  std::list<VoidFunc> blockedCallback_;
  SpinLock blockedLock_;
};

class FiberManager : noncopyable {
public:
  static void update(Fiber* fiber) {
    fiber_ = fiber;
  }

  static Fiber* get() {
    return fiber_;
  }

  static void run(Fiber* fiber) {
    update(fiber);
    Context ctx;
    fiber->setBackContext(&ctx);
    fiber->execute();
    switch (fiber->status()) {
      case Fiber::BLOCK:
        fiber->sweepBlockedCallback();
        break;
      case Fiber::INIT:
      case Fiber::RUNABLE:
      case Fiber::RUNNING:
        RDDLOG(WARN) << "fiber status error";
      case Fiber::EXIT:
      default:
        delete fiber;
        break;
    }
  }

  static bool yield() {
    Fiber* fiber = get();
    if (fiber) {
      fiber->yield(Fiber::BLOCK);
      return true;
    }
    return false;
  }

  static bool exit() {
    Fiber* fiber = get();
    if (fiber) {
      fiber->yield(Fiber::EXIT);
      return true;
    }
    return false;
  }

private:
  FiberManager() {}

  static __thread Fiber* fiber_;
};

}

