/*
 * Copyright (c) 2015, Facebook, Inc.
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
