/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdlib.h>

namespace rdd {

class DeleterBase {
public:
  virtual ~DeleterBase() {}
  virtual void dispose(void* ptr) const = 0;
};

template <class Ptr>
class SimpleDeleter {
public:
  virtual void dispose(void* ptr) const {
    delete static_cast<Ptr>(ptr);
  }
};

struct ContextWrapper {
  bool dispose() {
    if (ptr == nullptr) {
      return false;
    }
    deleter->dispose(ptr);
    cleanup();
    return true;
  }

  void* release() {
    auto retPtr = ptr;
    if (ptr != nullptr) {
      cleanup();
    }
    return retPtr;
  }

  template <class Ptr>
  void set(Ptr p) {
    if (p) {
      static auto d = new SimpleDeleter<Ptr>();
      ptr = p;
      deleter = d;
    }
  }

  void cleanup() {
    ptr = nullptr;
    deleter = nullptr;
  }

  void* ptr{nullptr};
  DeleterBase* deleter{nullptr};
};

}

