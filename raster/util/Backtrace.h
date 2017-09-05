/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <execinfo.h>
#include <unistd.h>

namespace rdd {

inline void recordBacktrace() {
  void* array[128];
  size_t size = backtrace(array, 128);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
}

} // namespace rdd
