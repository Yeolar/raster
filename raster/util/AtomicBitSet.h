/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>

#include "raster/util/Macro.h"

namespace rdd {

template <size_t N>
class AtomicBitSet {
 public:
  AtomicBitSet() : data_() {}

  bool set(size_t idx, std::memory_order order = std::memory_order_seq_cst) {
    assert(idx < N * kBlockBits);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].fetch_or(mask, order) & mask;
  }

  bool reset(size_t idx, std::memory_order order = std::memory_order_seq_cst) {
    assert(idx < N * kBlockBits);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].fetch_and(~mask, order) & mask;
  }

  bool set(size_t idx, bool value,
           std::memory_order order = std::memory_order_seq_cst) {
    return value ? set(idx, order) : reset(idx, order);
  }

  bool test(size_t idx,
            std::memory_order order = std::memory_order_seq_cst) const {
    assert(idx < N * kBlockBits);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].load(order) & mask;
  }

  bool operator[](size_t idx) const {
    return test(idx);
  }

  constexpr size_t size() const {
    return N;
  }

  NOCOPY(AtomicBitSet);

 private:
  typedef unsigned long long BlockType;   // lock free type
  typedef std::atomic<BlockType> AtomicBlockType;

  static constexpr size_t kBlockBits = std::numeric_limits<BlockType>::digits;
  static constexpr BlockType kOne = 1; // avoid casts

  static constexpr size_t blockIndex(size_t bit) {
    return bit / kBlockBits;
  }

  static constexpr size_t bitOffset(size_t bit) {
    return bit % kBlockBits;
  }

  std::array<AtomicBlockType, N> data_;
};

} // namespace rdd
