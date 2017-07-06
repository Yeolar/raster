/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <stdio.h>
#include <pthread.h>
#include "rddoc/util/Conv.h"
#include "rddoc/util/ThreadUtil.h"

#define RDD_DEBUG_LINE_(...) do { \
  std::mutex _dl_lock; \
  std::lock_guard<std::mutex> _dl_guard(_dl_lock); \
  printf("%d %s:%s:%d %s\n", \
         ::rdd::localThreadId(), __FILE__, __func__, __LINE__, \
         ::rdd::to<std::string>(__VA_ARGS__).c_str()); \
} while (0)

#define RDD_DEBUG_LINE RDD_DEBUG_LINE_("")

