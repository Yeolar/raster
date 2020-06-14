/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <list>
#include <memory>

#include <accelerator/Time.h>

#include "raster/coroutine/BoostContext.h"

namespace raster {

#define RASTER_FIBER_GEN(x) \
    x(Init),                \
    x(Runable),             \
    x(Running),             \
    x(Block),               \
    x(Exit)

#define RASTER_FIBER_ENUM(status) k##status

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
    RASTER_FIBER_GEN(RASTER_FIBER_ENUM)
  };

  static size_t count() { return count_; }

  Fiber(int stackSize, std::unique_ptr<Task> task);

  ~Fiber();

  Task* task() const;

  int status() const;
  void setStatus(int status);

  const char* statusName() const;

  void execute();
  void yield(int status);

  uint64_t startTime() const;
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
  std::vector<acc::StageTimestamp> timestamps_;
};

std::ostream& operator<<(std::ostream& os, const Fiber& fiber);

#undef RASTER_FIBER_ENUM

//////////////////////////////////////////////////////////////////////

inline Fiber::Task* Fiber::task() const {
  return task_.get();
}

inline int Fiber::status() const {
  return status_;
}

inline uint64_t Fiber::startTime() const {
  return timestamps_.front().stamp;
}

inline uint64_t Fiber::cost() const {
  return acc::elapsed(startTime());
}

} // namespace raster
