/*
 * Copyright 2017 Facebook, Inc.
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

#include "raster/util/Arena.h"

namespace rdd {

void* Arena::allocateSlow(size_t size) {
  std::pair<Block*, size_t> p;
  char* start;

  if (size > minBlockSize_) {
    p = Block::allocate(size);
    start = p.first->start();
    blocks_.push_back(*p.first);
  } else {
    p = Block::allocate(minBlockSize_);
    start = p.first->start();
    blocks_.push_front(*p.first);
    ptr_ = start + size;
    end_ = start + p.second;
  }

  assert(p.second >= size);
  totalAllocatedSize_ += p.second + sizeof(Block);
  return start;
}

Arena* ThreadArena::allocateThreadLocalArena() {
  Arena* arena = new Arena(minBlockSize_, maxAlign_);
  arena_.reset(arena);
  return arena;
}

} // namespace rdd
