/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <stdio.h>

#include "raster/util/Conv.h"
#include "raster/util/ThreadUtil.h"

#define RDD_DEBUG_LINE_(...) do { \
  std::mutex _dlLock; \
  std::lock_guard<std::mutex> _dlGuard(_dlLock); \
  printf("%d %s:%s:%d %s\n", \
         ::rdd::localThreadId(), __FILE__, __func__, __LINE__, \
         ::rdd::to<std::string>(__VA_ARGS__).c_str()); \
} while (0)

#define RDD_DEBUG_LINE RDD_DEBUG_LINE_("")

