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

#include "raster/coroutine/Fiber.h"

namespace raster {

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

} // namespace raster
