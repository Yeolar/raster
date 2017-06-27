/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/coroutine/Fiber.h"

namespace rdd {

std::atomic<size_t> Fiber::count_(0);

__thread Fiber* FiberManager::fiber_ = nullptr;

}

