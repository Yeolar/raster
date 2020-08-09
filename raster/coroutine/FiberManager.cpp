/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/coroutine/FiberManager.h"

#include "accelerator/Logging.h"

namespace raster {

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
      ACCLOG(WARN) << *fiber << " status error";
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

} // namespace raster
