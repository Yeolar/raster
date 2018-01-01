/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/coroutine/FiberManager.h"
#include "raster/util/Logging.h"

namespace rdd {

__thread Fiber* FiberManager::fiber_ = nullptr;

void FiberManager::update(Fiber* fiber) {
  fiber_ = fiber;
}

Fiber* FiberManager::get() {
  return fiber_;
}

void FiberManager::run(Fiber* fiber) {
  update(fiber);
  fiber->setStatus(Fiber::kRunable);
  fiber->execute();
  switch (fiber->status()) {
    case Fiber::kBlock: {
      for (auto& fn : fiber->task()->blockCallbacks) {
        fn();
      }
      break;
    }
    case Fiber::kExit: {
      fiber->task()->scheduleCallback();
      delete fiber;
      break;
    }
    default: {
      RDDLOG(WARN) << *fiber << " status error";
      delete fiber;
      break;
    }
  }
}

bool FiberManager::yield() {
  Fiber* fiber = get();
  if (fiber) {
    fiber->yield(Fiber::kBlock);
    return true;
  }
  return false;
}

bool FiberManager::exit() {
  Fiber* fiber = get();
  if (fiber) {
    fiber->yield(Fiber::kExit);
    return true;
  }
  return false;
}

Fiber::Task* getCurrentFiberTask() {
  Fiber* fiber = FiberManager::get();
  return fiber ? fiber->task() : nullptr;
}

} // namespace rdd
