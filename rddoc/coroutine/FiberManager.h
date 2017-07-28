/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/coroutine/Fiber.h"
#include "rddoc/util/Logging.h"

namespace rdd {

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
    fiber->setStatus(Fiber::RUNABLE);
    fiber->execute();
    switch (fiber->status()) {
      case Fiber::BLOCK:
        fiber->executor()->callback();
        break;
      case Fiber::INIT:
      case Fiber::RUNABLE:
      case Fiber::RUNNING:
        RDD_FCLOG(WARN, fiber) << "status error";
      case Fiber::EXIT:
        fiber->executor()->schedule();
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

inline ExecutorPtr getCurrentExecutor() {
  Fiber* fiber = FiberManager::get();
  return fiber ? fiber->executor() : nullptr;
}

} // namespace rdd
