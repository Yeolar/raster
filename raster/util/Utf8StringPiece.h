/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/util/Unicode.h"

namespace rdd {

namespace detail {

inline const char32_t& value_before(Utf8CharIterator i) {
  return *--i;
}

} // namespace detail

/**
 * Utf8StringPiece abstraction keeping a pair of UTF-8 iterators.
 *
 * (Keep memory lifetime in mind when using this class, since it
 * doesn't manage the data it refers to - just like an iterator
 * wouldn't.)
 */
class Utf8StringPiece : private boost::totally_ordered<Utf8StringPiece> {
 public:
  typedef size_t size_type;
  typedef Utf8CharIterator iterator;
  typedef Utf8CharIterator const_iterator;
  typedef const char32_t value_type;
  typedef const char32_t& reference;
  typedef std::char_traits<char> traits_type;

  static const size_t npos;

  Utf8StringPiece() : b_(), e_() {
  }

 public:
  Utf8StringPiece(Utf8CharIterator start, Utf8CharIterator end)
  : b_(start), e_(end) { }

  Utf8StringPiece(Utf8CharIterator start, size_t size)
  : b_(start), e_(&start + size) { }

  /* implicit */ Utf8StringPiece(Utf8CharIterator str)
  : b_(str), e_(&str + strlen(&str)) {}

  /* implicit */ Utf8StringPiece(const std::string& str)
  : b_(str.data()), e_(str.data() + str.size()) {}

  Utf8StringPiece(const std::string& str, size_t startFrom) {
    if (UNLIKELY(startFrom > str.size())) {
      throw std::out_of_range("index out of range");
    }
    b_ = str.data() + startFrom;
    e_ = str.data() + str.size();
  }

  Utf8StringPiece(const std::string& str,
                  size_t startFrom,
                  size_t size) {
    if (UNLIKELY(startFrom > str.size())) {
      throw std::out_of_range("index out of range");
    }
    b_ = str.data() + startFrom;
    if (str.size() - startFrom < size) {
      e_ = str.data() + str.size();
    } else {
      e_ = &b_ + size;
    }
  }

  /* implicit */ Utf8StringPiece(const StringPiece& str)
  : b_(str.start()), e_(str.end()) {}

  Utf8StringPiece(const Utf8StringPiece& str,
                  size_t startFrom,
                  size_t size) {
    if (UNLIKELY(startFrom > str.size())) {
      throw std::out_of_range("index out of range");
    }
    b_ = &str.b_ + startFrom;
    if (str.size() - startFrom < size) {
      e_ = str.e_;
    } else {
      e_ = &b_ + size;
    }
  }

  void clear() {
    b_ = Utf8CharIterator();
    e_ = Utf8CharIterator();
  }

  void assign(Utf8CharIterator start, Utf8CharIterator end) {
    b_ = start;
    e_ = end;
  }

  void reset(Utf8CharIterator start, size_t size) {
    b_ = start;
    e_ = &start + size;
  }

  void reset(const std::string& str) {
    reset(str.data(), str.size());
  }

  size_t size() const {
    assert(b_ <= e_);
    return &e_ - &b_;
  }

  size_t length() const {
    assert(b_ <= e_);
    return e_ - b_;
  }

  bool empty() const { return b_ == e_; }
  const char* data() const { return &b_; }
  Utf8CharIterator start() const { return b_; }
  Utf8CharIterator begin() const { return b_; }
  Utf8CharIterator end() const { return e_; }
  Utf8CharIterator cbegin() const { return b_; }
  Utf8CharIterator cend() const { return e_; }

  value_type& front() const {
    assert(b_ < e_);
    return *b_;
  }
  value_type& back() const {
    assert(b_ < e_);
    return detail::value_before(e_);
  }

  std::string str() const { return std::string(&b_, size()); }
  std::string toString() const { return str(); }

  int compare(const Utf8StringPiece& o) const {
    const size_t tsize = this->size();
    const size_t osize = o.size();
    const size_t msize = std::min(tsize, osize);
    int r = traits_type::compare(data(), o.data(), msize);
    if (r == 0) r = (int)tsize - (int)osize;
    return r;
  }

  void advance(size_t n) {
    iterator i = b_;
    while (n > 0 && i != e_) {
      ++i;
      --n;
    }
    if (UNLIKELY(n > 0)) {
      throw std::out_of_range("index out of range");
    }
    b_ = i;
  }

  void subtract(size_t n) {
    iterator i = e_;
    while (n > 0 && b_ != i) {
      --i;
      --n;
    }
    if (UNLIKELY(n > 0)) {
      throw std::out_of_range("index out of range");
    }
    e_ = i;
  }

  void pop_front() {
    assert(b_ < e_);
    ++b_;
  }

  void pop_back() {
    assert(b_ < e_);
    --e_;
  }

  void swap(Utf8StringPiece& rhs) {
    std::swap(b_, rhs.b_);
    std::swap(e_, rhs.e_);
  }

 private:
  iterator b_, e_;
};


inline void swap(Utf8StringPiece& lhs, Utf8StringPiece& rhs) {
  lhs.swap(rhs);
}

/**
 * Create a range from two iterators.
 */
inline Utf8StringPiece utf8Piece(Utf8CharIterator first,
                                 Utf8CharIterator last) {
  return Utf8StringPiece(first, last);
}

template <size_t n>
Utf8StringPiece utf8Piece(const char (&array)[n]) {
  return Utf8StringPiece(array, array + n);
}

/**
 * Comparison operators
 */

inline bool operator==(const Utf8StringPiece& lhs,
                       const Utf8StringPiece& rhs) {
  return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

inline bool operator<(const Utf8StringPiece& lhs,
                      const Utf8StringPiece& rhs) {
  return lhs.compare(rhs) < 0;
}

namespace detail {

template <class A, class B>
struct ComparableAsUtf8StringPiece {
  enum {
    value =
    (std::is_convertible<A, Utf8StringPiece>::value
     && std::is_same<B, Utf8StringPiece>::value)
    ||
    (std::is_convertible<B, Utf8StringPiece>::value
     && std::is_same<A, Utf8StringPiece>::value)
  };
};

} // namespace detail

/**
 * operator== through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator==(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) == Utf8StringPiece(rhs);
}

/**
 * operator!= through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator!=(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) != Utf8StringPiece(rhs);
}

/**
 * operator< through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator<(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) < Utf8StringPiece(rhs);
}

/**
 * operator> through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator>(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) > Utf8StringPiece(rhs);
}

/**
 * operator< through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator<=(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) <= Utf8StringPiece(rhs);
}

/**
 * operator> through conversion for Utf8StringPiece
 */
template <class T, class U>
typename std::enable_if<
  detail::ComparableAsUtf8StringPiece<T, U>::value,
  bool>::type
operator>=(const T& lhs, const U& rhs) {
  return Utf8StringPiece(lhs) >= Utf8StringPiece(rhs);
}

} // namespace rdd
