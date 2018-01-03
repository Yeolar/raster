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
  struct Task {
    virtual ~Task() {}
    virtual void handle() = 0;
    void run();

    Fiber* fiber;
    std::list<VoidFunc> blockCallbacks;
    VoidFunc scheduleCallback;
  };

 public:
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
