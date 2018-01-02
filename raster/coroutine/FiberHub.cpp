/*
 * Copyright (C) 2018, Yeolar
 */

#include "raster/coroutine/FiberHub.h"

#include <gflags/gflags.h>

#include "raster/coroutine/FiberManager.h"

DEFINE_uint64(fc_limit, 16384,      // 1GB / 64KB
              "Limit # of fiber context (warning).");

DEFINE_uint64(fc_stack_size, 65536, // 64KB
              "Stack size of fiber context.");

namespace rdd {

void FiberHub::execute(Fiber* fiber, int poolId) {
  auto executor = getCPUThreadPoolExecutor(poolId);
  RDDLOG(V2) << executor->getThreadFactory()->namePrefix()
             << "* add " << *fiber;
  executor->add([&]() { FiberManager::run(fiber); });
}

void FiberHub::execute(std::unique_ptr<Fiber::Task> task, int poolId) {
  Fiber* fiber = task->fiber;
  if (!fiber) {
    if (Fiber::count() >= FLAGS_fc_limit) {
      RDDLOG(WARN) << "exceed fiber capacity";
      // still add fiber
    }
    fiber = new Fiber(FLAGS_fc_stack_size, std::move(task));
  }
  execute(fiber, poolId);
}

} // namespace rdd
