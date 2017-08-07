/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/Executor.h"
#include "raster/coroutine/FiberManager.h"

namespace rdd {

class GenericExecutor : public Executor {
public:
  virtual ~GenericExecutor() {}

  virtual void handle() = 0;

  void run() {
    handle();
    FiberManager::exit();
  }
};

class FunctionExecutor : public GenericExecutor {
public:
  FunctionExecutor(VoidFunc&& func) : func_(func) {}

  virtual ~FunctionExecutor() {}

  void handle() {
    func_();
  }

private:
  VoidFunc func_;
};

} // namespace rdd
