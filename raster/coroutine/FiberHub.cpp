/*
 * Copyright 2018 Yeolar
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

#include "raster/coroutine/FiberHub.h"

#include "raster/Portability.h"
#include "raster/coroutine/FiberManager.h"

DEFINE_uint64(fc_limit, 16384,      // 1GB / 64KB
              "Limit # of fiber context (warning).");

DEFINE_uint64(fc_stack_size, 65536, // 64KB
              "Stack size of fiber context.");

namespace raster {

void FiberHub::execute(Fiber* fiber, int poolId) {
  auto executor = getCPUThreadPoolExecutor(poolId);
  ACCLOG(V2) << executor->getThreadFactory()->namePrefix()
             << "* add " << *fiber;
  executor->add([&]() { FiberManager::run(fiber); });
}

void FiberHub::execute(std::unique_ptr<Fiber::Task> task, int poolId) {
  Fiber* fiber = task->fiber;
  if (!fiber) {
    if (Fiber::count() >= FLAGS_fc_limit) {
      ACCLOG(WARN) << "exceed fiber capacity";
      // still add fiber
    }
    fiber = new Fiber(FLAGS_fc_stack_size, std::move(task));
  }
  execute(fiber, poolId);
}

} // namespace raster
