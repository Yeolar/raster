/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <assert.h>
#include <stddef.h>
#include <array>
#include <atomic>
#include <limits>
#include "rddoc/util/noncopyable.h"

namespace rdd {

template <size_t N>
class AtomicBitSet : noncopyable {
public:
  typedef unsigned long long BlockType;   // lock free type

  static const size_t BLOCK_BITS = std::numeric_limits<BlockType>::digits;

  AtomicBitSet() : data_() {}

  bool set(size_t idx) {
    assert(idx < N * BLOCK_BITS);
    BlockType mask = ONE << bitOffset(idx);
    return data_[blockIndex(idx)].fetch_or(mask) & mask;
  }

  bool reset(size_t idx) {
    assert(idx < N * BLOCK_BITS);
    BlockType mask = ONE << bitOffset(idx);
    return data_[blockIndex(idx)].fetch_and(~mask) & mask;
  }

  bool set(size_t idx, bool value) {
    return value ? set(idx) : reset(idx);
  }

  bool test(size_t idx) const {
    assert(idx < N * BLOCK_BITS);
    BlockType mask = ONE << bitOffset(idx);
    return data_[blockIndex(idx)].load() & mask;
  }

  bool operator[](size_t idx) const { return test(idx); }

  constexpr size_t size() const { return N; }
  constexpr size_t maskSize() const { return N * BLOCK_BITS; }

private:
  typedef std::atomic<BlockType> AtomicBlockType;

  static constexpr size_t blockIndex(size_t bit) {
    return bit / BLOCK_BITS;
  }

  static constexpr size_t bitOffset(size_t bit) {
    return bit % BLOCK_BITS;
  }

  static const BlockType ONE = 1; // avoid casts

  std::array<AtomicBlockType, N> data_;
};

} // namespace rdd
