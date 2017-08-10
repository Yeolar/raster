/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
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
  totalAllocatedSize_ += p.second + sizeof(Block);
  return start;
}

} // namespace rdd
