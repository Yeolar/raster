/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author Mark Rabkin (mrabkin@fb.com)
// @author Andrei Alexandrescu (andrei.alexandrescu@fb.com)

#include "rddoc/util/Range.h"

#include <iostream>

namespace rdd {

/**
 * Predicates that can be used with qfind and startsWith
 */
const AsciiCaseSensitive asciiCaseSensitive = AsciiCaseSensitive();
const AsciiCaseInsensitive asciiCaseInsensitive = AsciiCaseInsensitive();

namespace {

// It's okay if pages are bigger than this (as powers of two), but they should
// not be smaller.
constexpr size_t kMinPageSize = 4096;
static_assert(kMinPageSize >= 16,
              "kMinPageSize must be at least SSE register size");
#define PAGE_FOR(addr) \
  (reinterpret_cast<uintptr_t>(addr) / kMinPageSize)


// Aho, Hopcroft, and Ullman refer to this trick in "The Design and Analysis
// of Computer Algorithms" (1974), but the best description is here:
// http://research.swtch.com/sparse
class FastByteSet {
 public:
  FastByteSet() : size_(0) { }  // no init of arrays required!

  inline void add(uint8_t i) {
    if (!contains(i)) {
      dense_[size_] = i;
      sparse_[i] = size_;
      size_++;
    }
  }
  inline bool contains(uint8_t i) const {
    return sparse_[i] < size_ && dense_[sparse_[i]] == i;
  }

 private:
  uint16_t size_;  // can't use uint8_t because it would overflow if all
                   // possible values were inserted.
  uint8_t sparse_[256];
  uint8_t dense_[256];
};

}  // namespace

namespace detail {

size_t qfind_first_byte_of_byteset(const StringPiece haystack,
                                   const StringPiece needles) {
  FastByteSet s;
  for (auto needle: needles) {
    s.add(needle);
  }
  for (size_t index = 0; index < haystack.size(); ++index) {
    if (s.contains(haystack[index])) {
      return index;
    }
  }
  return StringPiece::npos;
}

size_t qfind_first_byte_of_nosse(const StringPiece haystack,
                                 const StringPiece needles) {
  if (UNLIKELY(needles.empty() || haystack.empty())) {
    return StringPiece::npos;
  }
  // The thresholds below were empirically determined by benchmarking.
  // This is not an exact science since it depends on the CPU, the size of
  // needles, and the size of haystack.
  if ((needles.size() >= 4 && haystack.size() <= 10) ||
             (needles.size() >= 16 && haystack.size() <= 64) ||
             needles.size() >= 32) {
    return qfind_first_byte_of_byteset(haystack, needles);
  }
  return qfind_first_of(haystack, needles, asciiCaseSensitive);
}

}  // namespace detail
}  // namespace rdd
