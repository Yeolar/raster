/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <unistd.h>
#include <sys/mman.h>
#include "raster/util/Exception.h"

namespace rdd {

class MemoryProtect {
public:
  static void* align(void* addr) {
    return (void*)(((size_t)addr) & ~(sysconf(_SC_PAGESIZE) - 1));
  }

  MemoryProtect() {}

  MemoryProtect(void* addr, size_t size = sysconf(_SC_PAGESIZE))
    : addr_(align(addr)), size_(size) {}

  void resize(size_t size) {
    size_ = size;
  }

  void set(int prot) {
    if (addr_ != nullptr) {
      checkUnixError(mprotect(addr_, size_, prot), "mprotect failed");
    }
  }

  void ban() {
    set(PROT_NONE);
  }

  void protect() {
    set(PROT_READ);
  }

  void unprotect() {
    set(PROT_READ|PROT_WRITE);
  }

private:
  void* addr_{nullptr};
  size_t size_{0};
};

} // namespace rdd
