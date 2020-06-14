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

#include "raster/coroutine/Fiber.h"

#include <accelerator/Logging.h>

#include "raster/coroutine/FiberManager.h"

#define RASTER_FIBER_STR(status) #status

namespace {

static const char* statusStrings[] = {
  RASTER_FIBER_GEN(RASTER_FIBER_STR)
};

}

namespace raster {

void Fiber::Task::run() {
  handle();
  FiberManager::exit();
}

std::atomic<size_t> Fiber::count_(0);

Fiber::Fiber(int stackSize, std::unique_ptr<Task> task)
    : task_(std::move(task)),
      stackLimit_(new unsigned char[stackSize]),
      stackSize_(stackSize),
      context_(std::bind(&Task::run, task_.get()), stackLimit_, stackSize_) {
  task_->fiber = this;
  timestamps_.push_back(acc::StageTimestamp(status_));
  ++count_;
}

Fiber::~Fiber() {
  delete [] stackLimit_;
  --count_;
}

void Fiber::setStatus(int status) {
  status_ = status;
  timestamps_.push_back(acc::StageTimestamp(status, cost()));
}

const char* Fiber::statusName() const {
  return statusStrings[status_];
}

void Fiber::execute() {
  assert(status_ == kRunable);
  setStatus(kRunning);
  ACCLOG(V5) << *this << " execute";
  context_.activate();
}

void Fiber::yield(int status) {
  assert(status_ == kRunning);
  setStatus(status);
  ACCLOG(V5) << *this << " yield";
  context_.deactivate();
}

std::string Fiber::timestampStr() const {
  std::vector<std::string> v;
  for (auto& ts : timestamps_) {
    v.push_back(ts.str());
  }
  return acc::join("-", v);
}

std::ostream& operator<<(std::ostream& os, const Fiber& fiber) {
  os << "fc("
     << (void*)(&fiber) << ","
     << fiber.statusName() << ","
     << fiber.timestampStr() << ")";
  return os;
}

} // namespace raster
