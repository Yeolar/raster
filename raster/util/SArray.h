/*
 * Copyright SRILM
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <assert.h>
#include <string.h>
#include <memory>
#include <vector>
#include "raster/util/Range.h"

namespace rdd {

/**
 * A smart array that retains shared ownership. It provides similar
 * functionalities comparing to std::vector, including data(), size(),
 * operator[], resize(), clear(). SArray can be easily constructed from
 * std::vector, such as
 *
 *    std::vector<int> a(10); SArray<int> b(a);  // copying
 *    std::shared_ptr<std::vector<int>> c(new std::vector<int>(10));
 *    SArray<int> d(c);  // only pointer copying
 *
 * SArray is also like a C pointer when copying and assigning, namely
 * both copy are assign are passing by pointers. The memory will be release only
 * if there is no copy exists. It is also can be cast without memory copy, such as
 *
 *    SArray<int> a(10);
 *    SArray<char> b(a);  // now b.size() = 10 * sizeof(int);
 */
template <class V>
class SArray {
public:
  SArray() {}
  ~SArray() {}

  /** Create an array with length n with initialized value. */
  explicit SArray(size_t size, V val = 0) {
    resize(size, val);
  }

  /**
   * construct from another SArray.
   * Zero-copy constructor, namely just copy the pointer
   */
  template <class W>
  explicit SArray(const SArray<W>& arr) {
    *this = arr;
  }

  /**
   * construct from another SArray.
   * Zero-copy constructor, namely just copy the pointer
   */
  template <class W>
  void operator=(const SArray<W>& arr) {
    size_ = arr.size() * sizeof(W) / sizeof(V);
    assert(size_ * sizeof(V) == arr.size() * sizeof(W));
    capacity_ = arr.capacity() * sizeof(W) / sizeof(V);
    ptr_ = std::shared_ptr<V>(arr.ptr(), reinterpret_cast<V*>(arr.data()));
  }

  /**
   * construct from a c-array.
   * Zero-copy constructor, namely just copy the pointer
   */
  SArray(V* data, size_t size, bool deletable = false) {
    if (deletable) {
      reset(data, size, [](V* data) { delete [] data; });
    } else {
      reset(data, size, [](V* data) {});
    }
  }

  /** copy from a c-array. */
  void copyFrom(const V* data, size_t size) {
    resize(size);
    memcpy(this->data(), data, size * sizeof(V));
  }

  /** copy from another SArray. */
  void copyFrom(const SArray<V>& other) {
    if (this == &other) return;
    copyFrom(other.data(), other.size());
  }

  /** copy from an iterator. */
  template <class It>
  void copyFrom(const It& first, const It& last) {
    int size = static_cast<int>(std::distance(first, last));
    V* data = new V[size];
    reset(data, size, [](V* data) { delete [] data; });
    auto it = first;
    while (size-- > 0) {
      *data++ = *it++;
    }
  }

  /** construct from a std::vector, copy the data. */
  explicit SArray(const std::vector<V>& vec) {
    copyFrom(vec.data(), vec.size());
  }

  /** construct from a shared std::vector pinter, no data copy. */
  explicit SArray(const std::shared_ptr<std::vector<V>>& vec) {
    ptr_ = std::shared_ptr<V>(vec, vec->data());
    size_ = capacity_ = vec->size();
  }

  /** Copy from a initializer_list. */
  template <class W> SArray(const std::initializer_list<W>& list) {
    copyFrom(list.begin(), list.end());
  }

  /** Copy from a initializer_list. */
  template <class W> void operator=(const std::initializer_list<W>& list) {
    copyFrom(list.begin(), list.end());
  }

  /** Reset the current data pointer with a deleter. */
  template <class Deleter>
  void reset(V* data, size_t size, Deleter del) {
    size_ = capacity_ = size;
    ptr_.reset(data, del);
  }

  /**
   * Resizes the array to size elements.
   *
   * If size <= capacity_, then only change the size. otherwise, append size -
   * current_size entries, and then set new value to val
   */
  void resize(size_t size, V val = 0) {
    size_t n = size_;
    if (capacity_ >= size) {
      size_ = size;
    } else {
      V* new_data = new V[size + 5];
      memcpy(new_data, data(), size_ * sizeof(V));
      reset(new_data, size, [](V* data) { delete [] data; });
    }
    if (size <= n) return;
    V* p = data() + n;
    if (val == 0) {
      memset(p, 0, (size - n) * sizeof(V));
    } else {
      for (size_t i = 0; i < size - n; ++i) {
        *p++ = val;
      }
    }
  }

  /**
   * Requests that the capacity be at least enough to contain n elements.
   */
  void reserve(size_t size) {
    if (capacity_ >= size) return;
    size_t old_size = size_;
    resize(size);
    size_ = old_size;
  }

  /** release the memory */
  void clear() {
    reset(nullptr, 0, [](V* data) {});
  }

  bool empty() const { return size() == 0; }
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }

  V* begin() { return data(); }
  const V* begin() const { return data(); }
  V* end() { return data() + size(); }
  const V* end() const { return data() + size(); }

  V* data() const { return ptr_.get(); }

  std::shared_ptr<V>& ptr() { return ptr_; }
  const std::shared_ptr<V>& ptr() const { return ptr_; }

  V front() const {
    assert(size_ != 0);
    return data()[0];
  }
  V back() const {
    assert(size_ != 0);
    return data()[size_ - 1];
  }

  V& operator[] (int i) { return data()[i]; }
  const V& operator[] (int i) const { return data()[i]; }

  void push_back(const V& val) {
    if (size_ == capacity_) reserve(size_ * 2 + 5);
    data()[size_++] = val;
  }

  void pop_back() {
    if (size_) --size_;
  }

  void append(const SArray<V>& arr) {
    if (arr.empty()) return;
    auto orig_size = size_;
    resize(size_ + arr.size());
    memcpy(data() + orig_size, arr.data(), arr.size() * sizeof(V));
  }

  /** Slice a segment, zero-copy */
  SArray<V> slice(size_t begin, size_t end) const {
    assert(end >= begin);
    assert(end <= size());
    SArray<V> ret;
    ret.ptr_ = std::shared_ptr<V>(ptr_, data() + begin);
    ret.size_ = end - begin;
    ret.capacity_ = end - begin;
    return ret;
  }

private:
  size_t size_{0};
  size_t capacity_{0};
  std::shared_ptr<V> ptr_;
};


/**
 * Find the index range of a slice of a sorted array such that the
 * entries in this slice is within [lower, upper). Assume
 * array values are ordered.
 *
 * An example
 *
 *    SArray<int> a{1 3 5 7 9};
 *    CHECK_EQ(Range(1,3), findRange(a, 2, 7);
 *
 */
template<class V>
Range<size_t> findRange(const SArray<V>& arr, V lower, V upper) {
  if (upper <= lower) return Range<size_t>(0, 0);
  auto lb = std::lower_bound(arr.begin(), arr.end(), lower);
  auto ub = std::lower_bound(arr.begin(), arr.end(), upper);
  return Range<size_t>(lb - arr.begin(), ub - arr.begin());
}

} // namespace rdd
