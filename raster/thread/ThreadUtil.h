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

#include <string>
#include <thread>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "raster/util/Macro.h"
#include "raster/util/Range.h"

namespace rdd {

/**
 * Get a process-specific identifier for the current thread.
 */
inline uint64_t currentThreadId() {
  return uint64_t(pthread_self());
}

/**
 * Get the operating-system level thread ID for the current thread.
 */
inline uint64_t osThreadId() {
  // __thread doesn't allow non-const initialization.
  static __thread uint64_t threadId = 0;
  if (UNLIKELY(threadId == 0)) {
    threadId = uint64_t(syscall(SYS_gettid));
  }
  return threadId;
}

/**
 * Get the name of the given thread.
 */
std::string getThreadName(pthread_t id);
std::string getThreadName(std::thread::id id);

std::string getCurrentThreadName();

/**
 * Set the name of the given thread.
 */
bool setThreadName(pthread_t id, StringPiece name);
bool setThreadName(std::thread::id id, StringPiece name);

bool setCurrentThreadName(StringPiece name);

template <class T>
class ThreadLocalPtr {
 public:
  ThreadLocalPtr() {
    pthread_key_create(&key_, OnThreadExit);
  }

  T* get() const {
    return static_cast<T*>(pthread_getspecific(key_));
  }

  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }

  void reset(T* t = nullptr) {
    delete get();
    pthread_setspecific(key_, t);
  }

  explicit operator bool() const {
    return get() != nullptr;
  }

  ThreadLocalPtr(const ThreadLocalPtr&) = delete;
  ThreadLocalPtr& operator=(const ThreadLocalPtr&) = delete;

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

  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }

  void reset(T* newPtr = nullptr) {
    tlp_.reset(newPtr);
  }

  ThreadLocal(const ThreadLocal&) = delete;
  ThreadLocal& operator=(const ThreadLocal&) = delete;

 private:
  T* makeTlp() const {
    T* ptr = new T();
    tlp_.reset(ptr);
    return ptr;
  }

  mutable ThreadLocalPtr<T> tlp_;
};

} // namespace rdd
