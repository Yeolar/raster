/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/Fiber.h"

namespace rdd {

class FunctionTask : public Fiber::Task {
 public:
  FunctionTask(VoidFunc&& func) : func_(std::move(func)) {}
  ~FunctionTask() override {}

  void handle() override {
    func_();
  }

 private:
  VoidFunc func_;
};

} // namespace rdd
