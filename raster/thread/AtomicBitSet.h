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

namespace rdd {

/**
 * An atomic bitset of fixed size (specified at compile time).
 */
template <size_t N>
class AtomicBitSet {
 public:
  /**
   * Construct an AtomicBitSet; all bits are initially false.
   */
  AtomicBitSet();

  /**
   * Set bit idx to true, using the given memory order. Returns the
   * previous value of the bit.
   *
   * Note that the operation is a read-modify-write operation due to the use
   * of fetch_or.
   */
  bool set(size_t idx, std::memory_order order = std::memory_order_seq_cst);

  /**
   * Set bit idx to false, using the given memory order. Returns the
   * previous value of the bit.
   *
   * Note that the operation is a read-modify-write operation due to the use
   * of fetch_and.
   */
  bool reset(size_t idx, std::memory_order order = std::memory_order_seq_cst);

  /**
   * Set bit idx to the given value, using the given memory order. Returns
   * the previous value of the bit.
   *
   * Note that the operation is a read-modify-write operation due to the use
   * of fetch_and or fetch_or.
   *
   * Yes, this is an overload of set(), to keep as close to std::bitset's
   * interface as possible.
   */
  bool set(size_t idx,
           bool value,
           std::memory_order order = std::memory_order_seq_cst);

  /**
   * Read bit idx.
   */
  bool test(size_t idx,
            std::memory_order order = std::memory_order_seq_cst) const;

  /**
   * Same as test() with the default memory order.
   */
  bool operator[](size_t idx) const;

  /**
   * Return the size of the bitset.
   */
  constexpr size_t size() const {
    return N;
  }

  AtomicBitSet(const AtomicBitSet&) = delete;
  AtomicBitSet& operator=(const AtomicBitSet&) = delete;

 private:
  typedef unsigned long long BlockType;
  typedef std::atomic<BlockType> AtomicBlockType;

  static constexpr size_t kBitsPerBlock =
    std::numeric_limits<BlockType>::digits;

  static constexpr size_t blockIndex(size_t bit) {
    return bit / kBitsPerBlock;
  }

  static constexpr size_t bitOffset(size_t bit) {
    return bit % kBitsPerBlock;
  }

  // avoid casts
  static constexpr BlockType kOne = 1;

  std::array<AtomicBlockType, N> data_;
};

// value-initialize to zero
template <size_t N>
inline AtomicBitSet<N>::AtomicBitSet() : data_() {
}

template <size_t N>
inline bool AtomicBitSet<N>::set(size_t idx, std::memory_order order) {
  assert(idx < N * kBitsPerBlock);
  BlockType mask = kOne << bitOffset(idx);
  return data_[blockIndex(idx)].fetch_or(mask, order) & mask;
}

template <size_t N>
inline bool AtomicBitSet<N>::reset(size_t idx, std::memory_order order) {
  assert(idx < N * kBitsPerBlock);
  BlockType mask = kOne << bitOffset(idx);
  return data_[blockIndex(idx)].fetch_and(~mask, order) & mask;
}

template <size_t N>
inline bool AtomicBitSet<N>::set(size_t idx,
                                 bool value,
                                 std::memory_order order) {
  return value ? set(idx, order) : reset(idx, order);
}

template <size_t N>
inline bool AtomicBitSet<N>::test(size_t idx, std::memory_order order) const {
  assert(idx < N * kBitsPerBlock);
  BlockType mask = kOne << bitOffset(idx);
  return data_[blockIndex(idx)].load(order) & mask;
}

template <size_t N>
inline bool AtomicBitSet<N>::operator[](size_t idx) const {
  return test(idx);
}

} // namespace rdd
