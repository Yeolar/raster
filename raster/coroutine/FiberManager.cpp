/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/coroutine/FiberManager.h"

namespace rdd {

__thread Fiber* FiberManager::fiber_ = nullptr;

} // namespace rdd
