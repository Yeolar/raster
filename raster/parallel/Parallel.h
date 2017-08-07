/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <functional>
#include <thread>
#include "raster/util/AssignOp.h"
#include "raster/util/Logging.h"
#include "raster/util/SArray.h"

namespace rdd {

namespace detail {

template<class T, class F>
void parallelSort(T* data, size_t len, size_t grainsize, const F& cmp) {
  if (len <= grainsize) {
    std::sort(data, data + len, cmp);
  } else {
    std::thread t(parallelSort<T, F>, data, len/2, grainsize, cmp);
    parallelSort(data + len/2, len - len/2, grainsize, cmp);
    t.join();
    std::inplace_merge(data, data + len/2, data + len, cmp);
  }
}

template <class K, class V>
void parallelOrderedMatch(
    const K* src_key, const K* src_key_end, const V* src_val,
    const K* dst_key, const K* dst_key_end, V* dst_val,
    int k, AssignOp op, size_t grainsize, size_t* n) {
  size_t src_len = std::distance(src_key, src_key_end);
  size_t dst_len = std::distance(dst_key, dst_key_end);
  if (dst_len == 0 || src_len == 0) return;

  // drop the unmatched tail of src
  src_key = std::lower_bound(src_key, src_key_end, *dst_key);
  src_val += (src_key - (src_key_end - src_len)) * k;

  if (dst_len <= grainsize) {
    while (dst_key != dst_key_end && src_key != src_key_end) {
      if (*src_key < *dst_key) {
        ++src_key;
        src_val += k;
      } else {
        if (!(*dst_key < *src_key)) {
          for (int i = 0; i < k; ++i) {
            assignOp(dst_val[i], src_val[i], op);
          }
          ++src_key;
          src_val += k;
          *n += k;
        }
        ++dst_key;
        dst_val += k;
      }
    }
  } else {
    std::thread t(
        parallelOrderedMatch<K, V>,
        src_key, src_key_end, src_val,
        dst_key, dst_key + dst_len / 2, dst_val,
        k, op, grainsize, n);
    size_t m = 0;
    parallelOrderedMatch<K, V>(
        src_key, src_key_end, src_val,
        dst_key + dst_len / 2, dst_key_end, dst_val + (dst_len / 2) * k,
        k, op, grainsize, &m);
    t.join();
    *n += m;
  }
}

} // namespace detail

/**
 * Parallel sort.
 */
template<class T, class F>
void parallelSort(SArray<T>* arr,
                  int num_threads = 2,
                  const F& cmp = std::less<T>()) {
  RDDCHECK_GT(num_threads, 0);
  RDDCHECK(cmp);
  size_t grainsize = std::max(arr->size() / num_threads + 5, (size_t)1024*16);
  detail::parallelSort(arr->data(), arr->size(), grainsize, cmp);
}

/**
 * Merge src_val into dst_val by matching keys. Keys must be unique and sorted.
 *
 *    if (dst_key[i] == src_key[j]) {
 *      dst_val[i] op= src_val[j]
 *    }
 *
 * When finished, dst_val will have length k * dst_key.size() and filled
 * with matched value. Umatched value will be untouched if exists
 * or filled with 0.
 */
template <class K, class V, class C>
size_t parallelOrderedMatch(
    const SArray<K>& src_key, const SArray<V>& src_val,
    const SArray<K>& dst_key, C& dst_val,
    int k = 1, AssignOp op = ASSIGN, int num_threads = 1) {
  RDDCHECK_GT(num_threads, 0);
  RDDCHECK_EQ(src_key.size() * k, src_val.size());

  dst_val.resize(dst_key.size() * k);
  if (dst_key.empty()) return 0;

  // shorten the matching range
  Range<size_t> range = findRange(dst_key, src_key.begin(), src_key.end());
  size_t grainsize = std::max(range.size() * k / num_threads + 5,
                              static_cast<size_t>(1024*1024));
  size_t n = 0;
  detail::parallelOrderedMatch<K, V>(
      src_key.begin(), src_key.end(), src_val.begin(),
      dst_key.begin() + range.begin(),
      dst_key.begin() + range.end(),
      dst_val.begin() + range.begin() * k,
      k, op, grainsize, &n);
  return n;
}

} // namespace rdd
