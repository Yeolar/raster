/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "rddoc/util/Macro.h"

namespace rdd {

typedef void* (*ThreadFn)(void*);

inline void createThread(ThreadFn fn, void* arg) {
  pthread_t tid;
  pthread_create(&tid, 0, fn, arg);
  pthread_detach(tid);
}

inline pid_t localThreadId() {
  // __thread doesn't allow non-const initialization.
  static __thread pid_t threadId = 0;
  if (UNLIKELY(threadId == 0)) {
    threadId = syscall(SYS_gettid);
  }
  return threadId;
}

template <typename T>
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
  static void OnThreadExit(void* obj) {
    delete static_cast<T*>(obj);
  }

private:
  pthread_key_t key_{0};
};

}
