/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <atomic>
#include "rddoc/coroutine/Context.h"
#include "rddoc/util/Function.h"
#include "rddoc/util/LockedMap.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/String.h"
#include "rddoc/util/ThreadUtil.h"

#define RDD_TTLOG(severity, t) \
  RDDLOG(severity) << "task(" << (void*)(t) << ", " \
    << (t)->type() << ", " \
    << (t)->timestampStr() << ") "

namespace rdd {

class Task : noncopyable {
public:
  enum {
    INIT,     // I
    RUNABLE,  // A
    RUNNING,  // R
    BLOCK,    // B
    EXIT,     // Q
    WAIT,     // W
  };

  enum {
    BASE,
    NET,
  };

  static size_t count() { return count_; }

  Task(int stack_size, int pid)
    : stack_size_(stack_size)
    , pid_(pid)
    , qstart_(timestampNow()) {
    timestamps_.push_back(Timestamp(status_));
    ++count_;
  }
  virtual ~Task() {
    --count_;
    RDD_TTLOG(V2, this) << "-";
  }

  template <class F, class... Ts>
  void set(F&& func, Ts&&... args) {
    context_.make(stack_size_, func, std::forward<Ts>(args)...);
  }

  virtual int type() const { return BASE; }
  virtual void close() {}
  virtual bool isConnectionClosed() { return false; }

  int pid() const { return pid_; }
  int status() const { return status_; }

  void setStatus(int status) {
    status_ = status;
    record(Timestamp(status_));
  }
  char statusLabel() const {
    return "IARBQW"[status_];
  }

  void setThreadContext(Context* context) {
    thread_context_ = context;
  }

  bool isTimeout() {
    if (qtimeout_ > 0) {
      uint64_t interval = timePassed(qstart_);
      if (interval > qtimeout_) {
        RDDLOG(WARN) << "task (" << (void*)this
          << ") is timeout(us): " << interval << ">" << qtimeout_;
        return true;
      }
    }
    return false;
  }

  void execute() {
    assert(thread_context_ != nullptr);
    assert(status_ == RUNABLE);
    status_ = RUNNING;
    record(Timestamp(status_));
    swapContext(thread_context_, &context_);
  }
  void yield(int status) {
    assert(thread_context_ != nullptr);
    assert(status_ == RUNNING);
    status_ = status;
    record(Timestamp(status_));
    context_.recordStackPosition();
    swapContext(&context_, thread_context_);
  }

  void addBlockedCallback(const VoidCallback& callback) {
    LockGuard guard(blocked_lock_);
    blocked_callback_.push_back(callback);
  }
  void sweepBlockedCallback() {
    LockGuard guard(blocked_lock_);
    for (auto& callback : blocked_callback_) {
      callback();
    }
    blocked_callback_.clear();
  }

  uint64_t starttime() const {
    return timestamps_.empty() ? 0 : timestamps_.front().stamp;
  }
  void record(Timestamp timestamp) {
    if (!timestamps_.empty()) {
      timestamp.stamp -= starttime();
    }
    timestamps_.push_back(timestamp);
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
  int stack_size_;
  int pid_;
  Context context_;
  Context* thread_context_{nullptr};

  uint64_t qstart_;           // deq start
  uint64_t qtimeout_{300000}; // deq timeout
  std::vector<Timestamp> timestamps_;

  std::vector<VoidCallback> blocked_callback_;
  Lock blocked_lock_;
};

class ThreadTask : noncopyable {
public:
  typedef LockedMap<pid_t, Task*> TaskMap;

  static void update(Task* task) {
    thread_tasks_.update(localThreadId(), task);
  }

  static Task* getCurrentThreadTask() {
    return thread_tasks_.get(localThreadId());
  }

private:
  ThreadTask() {}

  static TaskMap thread_tasks_;
};

inline bool yieldTask(bool block = true) {
  Task* task = ThreadTask::getCurrentThreadTask();
  if (task) {
    block ? task->yield(Task::BLOCK) : task->yield(Task::RUNABLE);
    return true;
  }
  return false;
}

inline bool exitTask() {
  Task* task = ThreadTask::getCurrentThreadTask();
  if (task) {
    task->yield(Task::EXIT);
    return true;
  }
  return false;
}

}

