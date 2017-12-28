/*
 * Copyright (C) 2017, Yeolar
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

void Fiber::Task::addBlockCallbacks(VoidFunc&& fn) {
  blockCallbacks_.push_back(std::move(fn));
}

void Fiber::Task::runBlockCallbacks() {
  for (auto& fn : blockCallbacks_) {
    fn();
  }
}

void Fiber::Task::addFinishCallbacks(VoidFunc&& fn) {
  finishCallbacks_.push_back(std::move(fn));
}

void Fiber::Task::runFinishCallbacks() {
  for (auto& fn : finishCallbacks_) {
    fn();
  }
}

std::atomic<size_t> Fiber::count_(0);

Fiber::Fiber(int stackSize, std::unique_ptr<Task> task)
  : task_(std::move(task)),
    stackLimit_(new unsigned char[stackSize]),
    stackSize_(stackSize),
    context_(std::bind(&Task::run, task_.get()), stackLimit_, stackSize_) {
  task_->setFiber(this);
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
