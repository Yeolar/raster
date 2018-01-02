/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/MMapIOAlloc.h"

#include <stdexcept>

namespace rdd {

namespace detail {

ByteRange findChunk(ByteRange range) {
  while (!range.empty()) {
    ChunkHead* h = (ChunkHead*) range.data();
    switch (h->flag) {
      case ChunkHead::INIT:
        return ByteRange();
      case ChunkHead::USED:
        return ByteRange(range.data() + sizeof(ChunkHead), h->size);
      case ChunkHead::FREE:
        range.advance(sizeof(ChunkHead) + h->size);
    }
  }
  return ByteRange();
}

} // namespace detail

MMapIO::Iterator::Iterator(ByteRange range, off_t pos)
  : range_(range),
    chunkAndPos_(ByteRange(), 0) {
  if (size_t(pos) >= range_.size()) {
    // Note that this branch can execute if pos is negative as well.
    chunkAndPos_.second = off_t(-1);
    range_.clear();
  } else {
    chunkAndPos_.second = pos;
    range_.advance(size_t(pos));
    advanceToValid();
  }
}

void MMapIO::Iterator::advanceToValid() {
  ByteRange chunk = detail::findChunk(range_);
  if (chunk.empty()) {
    chunkAndPos_ = std::make_pair(ByteRange(), off_t(-1));
    range_.clear();  // at end
  } else {
    size_t skipped = size_t(chunk.begin() - range_.begin());
    DCHECK_GE(skipped, sizeof(detail::ChunkHead));
    skipped -= sizeof(detail::ChunkHead);
    range_.advance(skipped);
    chunkAndPos_.first = chunk;
    chunkAndPos_.second += off_t(skipped);
  }
}

void MMapIO::init(uint16_t i) {
  RWSpinLock::WriteHolder guard(lock_);
  Head* h = head();
  h->index = i;
  h->flag = FREE;
  h->used = 0;
  h->tail = 0;
}

uint16_t MMapIO::index() const {
  RWSpinLock::ReadHolder guard(lock_);
  return head()->index;
}

bool MMapIO::isFree() const {
  RWSpinLock::ReadHolder guard(lock_);
  return head()->flag == FREE;
}

bool MMapIO::isFull() const {
  RWSpinLock::ReadHolder guard(lock_);
  return head()->flag == FULL;
}

float MMapIO::useRate() const {
  RWSpinLock::ReadHolder guard(lock_);
  return float(head()->used) / dataSize();
}

void* MMapIO::allocate(size_t size) {
  if (size > dataSize()) {
    throw std::runtime_error("too large size required");
  }
  RWSpinLock::WriteHolder guard(lock_);
  Head* h = head();
  if (size > dataSize() - h->tail) {
    h->flag = FULL;
    return nullptr;
  }
  void* p = data() + h->tail;
  h->used += size;
  h->tail += size;
  return p;
}

void MMapIO::deallocate(void* p, size_t size) {
  RWSpinLock::WriteHolder guard(lock_);
  Head* h = head();
  h->used -= size;
}

MMapIOPool::MMapIOPool(const Path& dir, size_t blockSize, uint16_t initCount)
  : dir_(dir), blockSize_(blockSize) {
  RDDCHECK(dir.isDirectory());
  pool_.resize(initCount + 1);
  for (size_t i = 1; i <= initCount; i++) {
    loadOrCreate(i);
  }
}

void* MMapIOPool::allocate(size_t size) {
  std::lock_guard<std::mutex> guard(lock_);
  try {
    return allocateOnFixed(size);
  } catch (std::bad_alloc&) {
    size_t n = pool_.size();
    pool_.resize(n + 1);
    loadOrCreate(n);
    return allocateOnFixed(size);
  }
  throw std::bad_alloc();
}

void MMapIOPool::deallocate(void* p, size_t size) {
  std::lock_guard<std::mutex> guard(lock_);
  detail::ChunkHead* h = (detail::ChunkHead*) p - 1;
  h->flag = detail::ChunkHead::FREE;
  pool_[h->index]->deallocate(p, h->size);
}

void MMapIOPool::loadOrCreate(uint16_t i) {
  auto m = new MMapIO(dir_ / to<std::string>(i), blockSize_);
  auto k = m->index();
  RDDCHECK(k == 0 || k == i);
  if (k == 0) {
    m->init(i);
  }
  pool_[i].reset(m);
  if (pool_[i]->isFree()) {
    frees_.push_back(i);
  }
}

void* MMapIOPool::allocateOnFixed(size_t size) {
  while (!frees_.empty()) {
    uint16_t i = frees_.front();
    void* p = pool_[i]->allocate(size + sizeof(detail::ChunkHead));
    if (p) {
      detail::ChunkHead* h = (detail::ChunkHead*) p;
      h->index = i;
      h->flag = detail::ChunkHead::USED;
      h->size = size;
      return h + 1;
    }
    frees_.pop_front();
  }
  throw std::bad_alloc();
}

} // namespace rdd
