/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <assert.h>
#include "rddoc/coroutine/Context.h"
#include "rddoc/coroutine/Executor.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/noncopyable.h"
#include "rddoc/util/Time.h"

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

  ExecutorPtr executor() const { return executor_; }

  void setExecutor(const ExecutorPtr& executor) {
    context_.make(stackSize_, &Executor::_, executor.get());
    executor_ = executor;
    executor_->fiber_ = this;
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
  ExecutorPtr executor_;

  uint64_t qstart_;           // deq start
  uint64_t qtimeout_{300000}; // deq timeout
  std::vector<Timestamp> timestamps_;
};

}

