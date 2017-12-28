/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/Fiber.h"

namespace rdd {

class FiberManager {
 public:
  static void update(Fiber* fiber);
  static Fiber* get();

  static void run(Fiber* fiber);
  static bool yield();
  static bool exit();

 private:
  FiberManager() {}

  FiberManager(const FiberManager&) = delete;
  FiberManager& operator=(const FiberManager&) = delete;

  static __thread Fiber* fiber_;
};

Fiber::Task* getCurrentFiberTask();

} // namespace rdd
