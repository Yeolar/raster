/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/coroutine/Executor.h"
#include "rddoc/coroutine/FiberManager.h"

namespace rdd {

class GenericExecutor : public Executor {
public:
  virtual ~GenericExecutor() {}

  virtual void handle() = 0;

  void run() {
    handle();
    FiberManager::exit();
  }

  template <class T>
  T* data() const { return reinterpret_cast<T*>(data_); }
  template <class T>
  void setData(T* ptr) { data_ = ptr; }

private:
  void* data_{nullptr};
};

class FunctionExecutor : public GenericExecutor {
public:
  FunctionExecutor(VoidFunc&& func) : func_(func) {}

  void handle() {
    func_();
  }

private:
  VoidFunc func_;
};

}

