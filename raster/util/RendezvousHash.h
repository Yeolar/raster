/*
 * Copyright (c) 2015, Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <vector>

namespace rdd {

class RendezvousHash {
 public:
  /**
   * build() builds the hashing pool based on a vector of nodes with
   * their keys and weights.
   */
  void build(std::vector<std::pair<std::string, uint64_t>>&);

  /**
   * get(key, N) finds the node ranked N in the consistent hashing space
   * for the given key.
   */
  size_t get(const uint64_t key, const size_t rank = 0) const;

  /**
   * get max error rate the current hashing space
   */
  double getMaxErrorRate() const;

 private:
  uint64_t computeHash(const char* data, size_t len) const;

  uint64_t computeHash(uint64_t i) const;

  std::vector<std::pair<uint64_t, uint64_t>> weights_;
};

} // namespace rdd
