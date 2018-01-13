/*
 * Copyright 2017 Facebook, Inc.
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

/**
 * Various low-level, bit-manipulation routines.
 *
 * findFirstSet(x)  [constexpr]
 *    find first (least significant) bit set in a value of an integral type,
 *    1-based (like ffs()).  0 = no bits are set (x == 0)
 *
 * findLastSet(x)  [constexpr]
 *    find last (most significant) bit set in a value of an integral type,
 *    1-based.  0 = no bits are set (x == 0)
 *    for x != 0, findLastSet(x) == 1 + floor(log2(x))
 *
 * nextPowTwo(x)  [constexpr]
 *    Finds the next power of two >= x.
 *
 * isPowTwo(x)  [constexpr]
 *    return true iff x is a power of two
 *
 * popcount(x)
 *    return the number of 1 bits in x
 *
 * Endian
 *    convert between native, big, and little endian representation
 *    Endian::big(x)      big <-> native
 *    Endian::little(x)   little <-> native
 *    Endian::swap(x)     big <-> little
 */

#pragma once

#include <cinttypes>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace rdd {

// Generate overloads for findFirstSet as wrappers around
// appropriate ffs, ffsl, ffsll gcc builtins
template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) <= sizeof(unsigned int)),
  unsigned int>::type
  findFirstSet(T x) {
  return static_cast<unsigned int>(__builtin_ffs(static_cast<int>(x)));
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>::type
  findFirstSet(T x) {
  return static_cast<unsigned int>(__builtin_ffsl(static_cast<long>(x)));
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned long) &&
   sizeof(T) <= sizeof(unsigned long long)),
  unsigned int>::type
  findFirstSet(T x) {
  return static_cast<unsigned int>(__builtin_ffsll(static_cast<long long>(x)));
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value && std::is_signed<T>::value),
  unsigned int>::type
  findFirstSet(T x) {
  // Note that conversion from a signed type to the corresponding unsigned
  // type is technically implementation-defined, but will likely work
  // on any impementation that uses two's complement.
  return findFirstSet(static_cast<typename std::make_unsigned<T>::type>(x));
}

// findLastSet: return the 1-based index of the highest bit set
// for x > 0, findLastSet(x) == 1 + floor(log2(x))
template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) <= sizeof(unsigned int)),
  unsigned int>::type
  findLastSet(T x) {
  // If X is a power of two X - Y = ((X - 1) ^ Y) + 1. Doing this transformation
  // allows GCC to remove its own xor that it adds to implement clz using bsr
  return x ? ((8 * sizeof(unsigned int) - 1) ^ __builtin_clz(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>::type
  findLastSet(T x) {
  return x ? ((8 * sizeof(unsigned long) - 1) ^ __builtin_clzl(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned long) &&
   sizeof(T) <= sizeof(unsigned long long)),
  unsigned int>::type
  findLastSet(T x) {
  return x ? ((8 * sizeof(unsigned long long) - 1) ^ __builtin_clzll(x)) + 1
           : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_signed<T>::value),
  unsigned int>::type
  findLastSet(T x) {
  return findLastSet(static_cast<typename std::make_unsigned<T>::type>(x));
}

template <class T>
inline constexpr
typename std::enable_if<
  std::is_integral<T>::value && std::is_unsigned<T>::value,
  T>::type
nextPowTwo(T v) {
  return v ? (T(1) << findLastSet(v - 1)) : 1;
}

template <class T>
inline constexpr
typename std::enable_if<
  std::is_integral<T>::value && std::is_unsigned<T>::value,
  bool>::type
isPowTwo(T v) {
  return (v != 0) && !(v & (v - 1));
}

/**
 * Population count
 */
template <class T>
inline typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) <= sizeof(unsigned int)),
  size_t>::type
  popcount(T x) {
  return size_t(__builtin_popcount(x));
}

template <class T>
inline typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long long)),
  size_t>::type
  popcount(T x) {
  return size_t(__builtin_popcountll(x));
}

/**
 * Endianness detection and manipulation primitives.
 */
namespace detail {

template <class T>
struct EndianIntBase {
 public:
  static T swap(T x);
};

#define RDD_GEN(t, fn) \
template<> inline t EndianIntBase<t>::swap(t x) { return fn(x); }

// fn(x) expands to (x) if the second argument is empty, which is exactly
// what we want for [u]int8_t.
RDD_GEN( int8_t,)
RDD_GEN(uint8_t,)
RDD_GEN( int64_t, __builtin_bswap64)
RDD_GEN(uint64_t, __builtin_bswap64)
RDD_GEN( int32_t, __builtin_bswap32)
RDD_GEN(uint32_t, __builtin_bswap32)
RDD_GEN( int16_t, __builtin_bswap16)
RDD_GEN(uint16_t, __builtin_bswap16)

#undef RDD_GEN

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

template <class T>
struct EndianInt : public detail::EndianIntBase<T> {
 public:
  static T big(T x) { return EndianInt::swap(x); }
  static T little(T x) { return x; }
};

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

template <class T>
struct EndianInt : public detail::EndianIntBase<T> {
 public:
  static T big(T x) { return x; }
  static T little(T x) { return EndianInt::swap(x); }
};

#else
# error Your machine uses a weird endianness!
#endif  /* __BYTE_ORDER__ */

} // namespace detail

// big* convert between native and big-endian representations
// little* convert between native and little-endian representations
// swap* convert between big-endian and little-endian representations
//
// ntohs, htons == big16
// ntohl, htonl == big32
#define RDD_GEN1(fn, t, sz) \
  static t fn##sz(t x) { return fn<t>(x); } \

#define RDD_GEN2(t, sz) \
  RDD_GEN1(swap, t, sz) \
  RDD_GEN1(big, t, sz) \
  RDD_GEN1(little, t, sz)

#define RDD_GEN(sz) \
  RDD_GEN2(uint##sz##_t, sz) \
  RDD_GEN2(int##sz##_t, sz)

class Endian {
 public:
  enum class Order : uint8_t {
    LITTLE,
    BIG
  };

  static constexpr Order order =
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    Order::LITTLE;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    Order::BIG;
#else
# error Your machine uses a weird endianness!
#endif  /* __BYTE_ORDER__ */

  template <class T> static T swap(T x) {
    return detail::EndianInt<T>::swap(x);
  }
  template <class T> static T big(T x) {
    return detail::EndianInt<T>::big(x);
  }
  template <class T> static T little(T x) {
    return detail::EndianInt<T>::little(x);
  }

  RDD_GEN(64)
  RDD_GEN(32)
  RDD_GEN(16)
  RDD_GEN(8)
};

#undef RDD_GEN
#undef RDD_GEN2
#undef RDD_GEN1

template <class T, class Enable=void> struct Unaligned;

/**
 * Representation of an unaligned value of a POD type.
 */
template <class T>
struct Unaligned<
    T,
    typename std::enable_if<std::is_pod<T>::value>::type> {
  Unaligned() = default;  // uninitialized
  /* implicit */ Unaligned(T v) : value(v) { }
  T value;
} __attribute__((__packed__));

/**
 * Read an unaligned value of type T and return it.
 */
template <class T>
inline T loadUnaligned(const void* p) {
  static_assert(sizeof(Unaligned<T>) == sizeof(T), "Invalid unaligned size");
  static_assert(alignof(Unaligned<T>) == 1, "Invalid alignment");
  return static_cast<const Unaligned<T>*>(p)->value;
}

/**
 * Write an unaligned value of type T.
 */
template <class T>
inline void storeUnaligned(void* p, T value) {
  static_assert(sizeof(Unaligned<T>) == sizeof(T), "Invalid unaligned size");
  static_assert(alignof(Unaligned<T>) == 1, "Invalid alignment");
  new (p) Unaligned<T>(value);
}

} // namespace rdd
