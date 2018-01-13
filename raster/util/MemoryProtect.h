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

#include <sys/mman.h>

#include "raster/util/Exception.h"
#include "raster/util/Memory.h"

namespace rdd {

class MemoryProtect {
 public:
  MemoryProtect() {}

  MemoryProtect(void* addr, size_t size = sysconf(_SC_PAGESIZE))
    : addr_(alignAddress(addr)), size_(size) {}

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
    set(PROT_READ | PROT_WRITE);
  }

 private:
  void* addr_{nullptr};
  size_t size_{0};
};

} // namespace rdd
