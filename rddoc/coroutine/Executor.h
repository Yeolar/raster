/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <list>
#include <memory>
#include "rddoc/util/Function.h"

namespace rdd {

class Fiber;

class Executor {
public:
  virtual ~Executor() {}

  virtual void run() = 0;

  void addCallback(VoidFunc&& callback) {
    callbacks_.emplace_back(std::move(callback));
  }
  void callback() {
    for (auto& f : callbacks_) { f(); }
  }

  void addSchedule(VoidFunc&& schedule) {
    schedules_.emplace_back(std::move(schedule));
  }
  void schedule() {
    for (auto& f : schedules_) { f(); }
  }

  Fiber* fiber{nullptr};

private:
  std::list<VoidFunc> callbacks_;
  std::list<VoidFunc> schedules_;
};

typedef std::shared_ptr<Executor> ExecutorPtr;

} // namespace rdd
