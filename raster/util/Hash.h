/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include <stdint.h>
#include <functional>
#include <iterator>
#include <utility>

namespace rdd {
namespace hash {

// This is the Hash128to64 function from Google's cityhash (available
// under the MIT License).  We use it to reduce multiple 64 bit hashes
// into a single hash.
inline uint64_t hash_128_to_64(const uint64_t upper, const uint64_t lower) {
  // Murmur-inspired hashing.
  const uint64_t kMul = 0x9ddfea08eb382d69ULL;
  uint64_t a = (lower ^ upper) * kMul;
  a ^= (a >> 47);
  uint64_t b = (upper ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}

// Never used, but gcc demands it.
template <class Hasher>
inline size_t hash_combine_generic() {
  return 0;
}

template <
  class Iter,
  class Hash = std::hash<typename std::iterator_traits<Iter>::value_type>>
uint64_t hash_range(Iter begin, Iter end,
          uint64_t hash = 0,
          Hash hasher = Hash()) {
  for (; begin != end; ++begin) {
    hash = hash_128_to_64(hash, hasher(*begin));
  }
  return hash;
}

template <class Hasher, typename T, typename... Ts>
size_t hash_combine_generic(const T& t, const Ts&... ts) {
  size_t seed = Hasher::hash(t);
  if (sizeof...(ts) == 0) {
    return seed;
  }
  size_t remainder = hash_combine_generic<Hasher>(ts...);
  return hash_128_to_64(seed, remainder);
}

class StdHasher {
public:
  template <typename T>
  static size_t hash(const T& t) {
    return std::hash<T>()(t);
  }
};

template <typename T, typename... Ts>
size_t hash_combine(const T& t, const Ts&... ts) {
  return hash_combine_generic<StdHasher>(t, ts...);
}

/*
 * Fowler / Noll / Vo (FNV) Hash
 *     http://www.isthe.com/chongo/tech/comp/fnv/
 */

const uint32_t FNV_32_HASH_START = 2166136261UL;
const uint64_t FNV_64_HASH_START = 14695981039346656037ULL;

inline uint32_t fnv32(const char* s,
                      uint32_t hash = FNV_32_HASH_START) {
  for (; *s; ++s) {
    hash += (hash << 1) + (hash << 4) + (hash << 7) +
            (hash << 8) + (hash << 24);
    hash ^= *s;
  }
  return hash;
}

inline uint32_t fnv32_buf(const void* buf,
                          size_t n,
                          uint32_t hash = FNV_32_HASH_START) {
  const char* char_buf = reinterpret_cast<const char*>(buf);

  for (size_t i = 0; i < n; ++i) {
    hash += (hash << 1) + (hash << 4) + (hash << 7) +
            (hash << 8) + (hash << 24);
    hash ^= char_buf[i];
  }

  return hash;
}

inline uint32_t fnv32(const std::string& str,
                      uint32_t hash = FNV_32_HASH_START) {
  return fnv32_buf(str.data(), str.size(), hash);
}

inline uint64_t fnv64(const char* s,
                      uint64_t hash = FNV_64_HASH_START) {
  for (; *s; ++s) {
    hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) +
            (hash << 8) + (hash << 40);
    hash ^= *s;
  }
  return hash;
}

inline uint64_t fnv64_buf(const void* buf,
                          size_t n,
                          uint64_t hash = FNV_64_HASH_START) {
  const char* char_buf = reinterpret_cast<const char*>(buf);

  for (size_t i = 0; i < n; ++i) {
    hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) +
            (hash << 8) + (hash << 40);
    hash ^= char_buf[i];
  }
  return hash;
}

inline uint64_t fnv64(const std::string& str,
                      uint64_t hash = FNV_64_HASH_START) {
  return fnv64_buf(str.data(), str.size(), hash);
}

} // namespace hash

// recursion
template <size_t index, typename... Ts>
struct TupleHasher {
  size_t operator()(std::tuple<Ts...> const& key) const {
    return hash::hash_combine(
      TupleHasher<index - 1, Ts...>()(key),
      std::get<index>(key));
  }
};

// base
template <typename... Ts>
struct TupleHasher<0, Ts...> {
  size_t operator()(std::tuple<Ts...> const& key) const {
    // we could do std::hash here directly, but hash_combine hides all the
    // ugly templating implicitly
    return hash::hash_combine(std::get<0>(key));
  }
};

} // namespace rdd

namespace std {

// Hash function for pairs. Requires default hash functions for both
// items in the pair.
template <typename T1, typename T2>
struct hash<std::pair<T1, T2>> {
public:
  size_t operator()(const std::pair<T1, T2>& x) const {
    return rdd::hash::hash_combine(x.first, x.second);
  }
};

// Hash function for tuples. Requires default hash functions for all types.
template <typename... Ts>
struct hash<std::tuple<Ts...>> {
  size_t operator()(std::tuple<Ts...> const& key) const {
    rdd::TupleHasher<
      std::tuple_size<std::tuple<Ts...>>::value - 1, // start index
      Ts...> hasher;
    return hasher(key);
  }
};

} // namespace std
