/*
 * Copyright (C) 2017, Yeolar
 */

#include <cstring>
#include <type_traits>

#include "raster/thread/ThreadUtil.h"

namespace rdd {

namespace {

pthread_t stdTidToPthreadId(std::thread::id tid) {
  static_assert(
      std::is_same<pthread_t, std::thread::native_handle_type>::value,
      "This assumes that the native handle type is pthread_t");
  static_assert(
      sizeof(std::thread::native_handle_type) == sizeof(std::thread::id),
      "This assumes std::thread::id is a thin wrapper around "
      "std::thread::native_handle_type, but that doesn't appear to be true.");
  pthread_t id;
  std::memcpy(&id, &tid, sizeof(id));
  return id;
}

} // namespace

static constexpr size_t kMaxThreadNameLength = 16;

std::string getThreadName(pthread_t id) {
  char buf[kMaxThreadNameLength];
  if (pthread_getname_np(id, buf, kMaxThreadNameLength) == 0) {
    return buf;
  }
  return "";
}

std::string getThreadName(std::thread::id id) {
  return getThreadName(stdTidToPthreadId(id));
}

std::string getCurrentThreadName() {
  return getThreadName(std::this_thread::get_id());
}

bool setThreadName(pthread_t id, StringPiece name) {
  name = name.subpiece(0, kMaxThreadNameLength - 1);
  char buf[kMaxThreadNameLength] = {};
  std::memcpy(buf, name.data(), name.size());
  return pthread_setname_np(id, buf) == 0;
}

bool setThreadName(std::thread::id id, StringPiece name) {
  return setThreadName(stdTidToPthreadId(id), name);
}

bool setCurrentThreadName(StringPiece name) {
  return setThreadName(std::this_thread::get_id(), name);
}

} // namespace rdd
