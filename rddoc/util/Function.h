/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <functional>

namespace rdd {

typedef std::function<void(void)> VoidFunc;
typedef std::function<void(void*)> PtrFunc;

} // namespace rdd
