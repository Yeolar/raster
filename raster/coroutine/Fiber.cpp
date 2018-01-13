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

#include "raster/coroutine/FiberManager.h"
#include "raster/util/Logging.h"

#define RDD_FIBER_STR(status) #status

namespace {
  static const char* statusStrings[] = {
    RDD_FIBER_GEN(RDD_FIBER_STR)
  };
}

namespace rdd {

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
  timestamps_.push_back(std::move(Timestamp(status_)));
  ++count_;
}

Fiber::~Fiber() {
  delete [] stackLimit_;
  --count_;
}

void Fiber::setStatus(int status) {
  status_ = status;
  timestamps_.push_back(std::move(Timestamp(status, cost())));
}

const char* Fiber::statusName() const {
  return statusStrings[status_];
}

void Fiber::execute() {
  assert(status_ == kRunable);
  setStatus(kRunning);
  RDDLOG(V5) << *this << " execute";
  context_.activate();
}

void Fiber::yield(int status) {
  assert(status_ == kRunning);
  setStatus(status);
  RDDLOG(V5) << *this << " yield";
  context_.deactivate();
}

uint64_t Fiber::starttime() const {
  return timestamps_.front().stamp;
}

uint64_t Fiber::cost() const {
  return timePassed(starttime());
}

std::string Fiber::timestampStr() const {
  return join("-", timestamps_);
}

std::ostream& operator<<(std::ostream& os, const Fiber& fiber) {
  os << "fc("
     << (void*)(&fiber) << ", "
     << fiber.statusName() << ", "
     << fiber.timestampStr() << ")";
  return os;
}

} // namespace rdd
