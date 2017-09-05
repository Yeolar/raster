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
  MemoryProtect(void* addr)
    : addr_(addr),
      size_(sysconf(_SC_PAGESIZE)) {}

  void protect() {
    checkUnixError(mprotect(addr_, size_, PROT_READ),
                   "mprotect failed");
  }

  void unprotect() {
    checkUnixError(mprotect(addr_, size_, PROT_READ|PROT_WRITE),
                   "mprotect failed");
  }

private:
  void* addr_;
  size_t size_;
};

} // namespace rdd
