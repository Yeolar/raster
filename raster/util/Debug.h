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

#include <cstdio>
#include <mutex>

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

