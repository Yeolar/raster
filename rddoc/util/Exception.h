/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <errno.h>
#include <system_error>
#include "rddoc/util/Conv.h"

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
