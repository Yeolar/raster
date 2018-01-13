/*
 * Copyright 2017 Facebook, Inc.
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

#include <cerrno>
#include <system_error>

#include "raster/util/Conv.h"

namespace rdd {

// Helper to throw std::system_error
inline void throwSystemErrorExplicit(int err, const char* msg) {
  throw std::system_error(err, std::system_category(), msg);
}

template <class... Args>
void throwSystemErrorExplicit(int err, Args&&... args) {
  throwSystemErrorExplicit(
    err, to<std::string>(std::forward<Args>(args)...).c_str());
}

template <class... Args>
void throwSystemError(Args&&... args) {
  throwSystemErrorExplicit(errno, std::forward<Args>(args)...);
}

template <class... Args>
void checkPosixError(int err, Args&&... args) {
  if (UNLIKELY(err != 0)) {
    throwSystemErrorExplicit(err, std::forward<Args>(args)...);
  }
}

template <class... Args>
void checkUnixError(ssize_t ret, Args&&... args) {
  if (UNLIKELY(ret == -1)) {
    throwSystemError(std::forward<Args>(args)...);
  }
}

template <class... Args>
void checkFopenError(FILE* fp, Args&&... args) {
  if (UNLIKELY(!fp)) {
    throwSystemError(std::forward<Args>(args)...);
  }
}

template <typename E, typename V, typename... Args>
void throwOnFail(V&& value, Args&&... args) {
  if (!value) {
    throw E(std::forward<Args>(args)...);
  }
}

/**
 * If cond is not true, raise an exception of type E.  E must have a ctor that
 * works with const char* (a description of the failure).
 */
#define CHECK_THROW(cond, E) \
  ::rdd::throwOnFail<E>((cond), "Check failed: " #cond)

} // namespace rdd
