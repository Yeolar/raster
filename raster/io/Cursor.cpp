/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Cursor.h"

#include <cstdio>

#include "raster/util/ScopeGuard.h"

namespace rdd {
namespace io {

void Appender::printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void Appender::vprintf(const char* fmt, va_list ap) {
  // Make a copy of ap in case we need to retry.
  // We use ap on the first attempt, so it always gets advanced
  // passed the used arguments.  We'll only use apCopy if we need to retry.
  va_list apCopy;
  va_copy(apCopy, ap);
  SCOPE_EXIT {
    va_end(apCopy);
  };

  // First try writing into our available data space.
  int ret = vsnprintf(reinterpret_cast<char*>(writableData()), length(),
                      fmt, ap);
  if (ret < 0) {
    throw std::runtime_error("error formatting printf() data");
  }
  auto len = size_t(ret);
  // vsnprintf() returns the number of characters that would be printed,
  // not including the terminating nul.
  if (len < length()) {
    // All of the data was successfully written.
    append(len);
    return;
  }

  // There wasn't enough room for the data.
  // Allocate more room, and then retry.
  ensure(len + 1);
  ret = vsnprintf(reinterpret_cast<char*>(writableData()), length(),
                  fmt, apCopy);
  if (ret < 0) {
    throw std::runtime_error("error formatting printf() data");
  }
  len = size_t(ret);
  if (len >= length()) {
    // This shouldn't ever happen.
    throw std::runtime_error("unexpectedly out of buffer space on second "
                             "vsnprintf() attmept");
  }
  append(len);
}

} // namespace io
} // namespace rdd
