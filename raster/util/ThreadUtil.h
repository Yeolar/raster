/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "raster/util/Macro.h"

namespace rdd {

inline pid_t localThreadId() {
  // __thread doesn't allow non-const initialization.
  static __thread pid_t threadId = 0;
  if (UNLIKELY(threadId == 0)) {
    threadId = syscall(SYS_gettid);
  }
  return threadId;
}

inline bool setThreadName(pthread_t id, const std::string& name) {
  return pthread_setname_np(id, name.substr(0, 15).c_str()) == 0;
}

template <class T>
class ThreadLocalPtr {
public:
  ThreadLocalPtr() {
    pthread_key_create(&key_, OnThreadExit);
  }

  T* get() const {
    return static_cast<T*>(pthread_getspecific(key_));
  }

  void reset(T* t) {
    delete get();
    pthread_setspecific(key_, t);
  }

  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }

  explicit operator bool() const { return get() != nullptr; }

private:
  static void OnThreadExit(void* obj) {
    delete static_cast<T*>(obj);
  }

  pthread_key_t key_{0};
};

template <class T>
class ThreadLocal {
public:
  ThreadLocal() {}

  T* get() const {
    T* ptr = tlp_.get();
    if (LIKELY(ptr != nullptr)) {
      return ptr;
    }
    return makeTlp();
  }

  void reset(T* newPtr = nullptr) {
    tlp_.reset(newPtr);
  }

  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }

  NOCOPY(ThreadLocal);

private:
  T* makeTlp() const {
    T* ptr = new T();
    tlp_.reset(ptr);
    return ptr;
  }

  mutable ThreadLocalPtr<T> tlp_;
};

} // namespace rdd
