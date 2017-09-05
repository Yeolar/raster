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
  MemoryProtect(void* addr, bool align = true)
    : addr_(addr),
      size_(sysconf(_SC_PAGESIZE)) {
    if (align) {
      addr_ = (void*)(((size_t)addr_) & ~(size_ - 1));
    }
  }

  void set(int prot) {
    checkUnixError(mprotect(addr_, size_, prot), "mprotect failed");
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
  void* addr_;
  size_t size_;
};

} // namespace rdd
