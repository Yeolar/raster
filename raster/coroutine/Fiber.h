/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <cassert>
#include <list>
#include <memory>

#include "raster/coroutine/BoostContext.h"
#include "raster/util/Time.h"

namespace rdd {

#define RDD_FIBER_GEN(x) \
    x(Init),             \
    x(Runable),          \
    x(Running),          \
    x(Block),            \
    x(Exit)

#define RDD_FIBER_ENUM(status) k##status

class Fiber {
 public:
  class Task {
   public:
    virtual ~Task() {}

    virtual void handle() = 0;

    void run();

    void setFiber(Fiber* fiber) { fiber_ = fiber; }
    Fiber* fiber() const { return fiber_; }

    void addBlockCallbacks(VoidFunc&& fn);
    void runBlockCallbacks();

    void addFinishCallbacks(VoidFunc&& fn);
    void runFinishCallbacks();

   private:
    Fiber* fiber_;
    std::list<VoidFunc> blockCallbacks_;
    std::list<VoidFunc> finishCallbacks_;
  };

  enum Status {
    RDD_FIBER_GEN(RDD_FIBER_ENUM)
  };

  static size_t count() { return count_; }

  Fiber(int stackSize, std::unique_ptr<Task> task);

  ~Fiber();

  Task* task() const { return task_.get(); }

  int status() const { return status_; }
  void setStatus(int status);

  const char* statusName() const;

  void execute();
  void yield(int status);

  uint64_t starttime() const;
  uint64_t cost() const;

  std::string timestampStr() const;

  Fiber(const Fiber&) = delete;
  Fiber& operator=(const Fiber&) = delete;

 private:
  static std::atomic<size_t> count_;

  std::unique_ptr<Task> task_;

  unsigned char* stackLimit_;
  size_t stackSize_;
  Context context_;

  int status_{kInit};
  std::vector<Timestamp> timestamps_;
};

std::ostream& operator<<(std::ostream& os, const Fiber& fiber);

#undef RDD_FIBER_ENUM

} // namespace rdd
