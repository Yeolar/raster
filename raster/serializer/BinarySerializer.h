/*
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

#include <map>
#include <set>
#include <string>
#include <type_traits>

#include "raster/util/Macro.h"
#include "raster/util/Range.h"

namespace rdd {

/*
 * Binary serialize protocol.
 * Support basic types, most STL containers, and self-defined structs,
 * can be extended by adding serialize/unserialize functions.
 */

template <class T>
inline typename std::enable_if<std::is_arithmetic<T>::value, uint32_t>::type
unserialize(ByteRange in, uint32_t pos, T& value) {
  memcpy(&value, in.data() + pos, sizeof(T));
  return sizeof(T);
}

template <class Out>
inline void serialize(const std::string& value, Out& out) {
  uint32_t n = value.size();
  serialize(n, out);
  out.append(value);
}

inline uint32_t unserialize(ByteRange in, uint32_t pos, std::string& value) {
  uint32_t n = 0;
  uint32_t p = pos + unserialize(in, pos, n);
  value.assign((char*)in.data() + p, n);
  return n + sizeof(uint32_t);
}

template <class T, class Out>
void serialize(const std::vector<T>& value, Out& out) {
  uint32_t n = value.size();
  serialize(n, out);
  for (auto& i : value) {
    serialize(i, out);
  }
}

template <class T>
uint32_t unserialize(ByteRange in, uint32_t pos, std::vector<T>& value) {
  uint32_t n = 0;
  uint32_t p = pos + unserialize(in, pos, n);
  value.clear();
  value.reserve(n);
  T v;
  for (uint32_t i = 0; i < n; i++) {
    p += unserialize(in, p, v);
    value.push_back(std::move(v));
  }
  return p - pos;
}

template <class T, class Out>
void serialize(const std::set<T>& value, Out& out) {
  uint32_t n = value.size();
  serialize(n, out);
  for (auto& i : value) {
    serialize(i, out);
  }
}

template <class T>
uint32_t unserialize(ByteRange in, uint32_t pos, std::set<T>& value) {
  uint32_t n = 0;
  uint32_t p = pos + unserialize(in, pos, n);
  value.clear();
  T v;
  for (uint32_t i = 0; i < n; i++) {
    p += unserialize(in, p, v);
    value.insert(std::move(v));
  }
  return p - pos;
}

template <class K, class V, class Out>
void serialize(const std::map<K, V>& value, Out& out) {
  uint32_t n = value.size();
  serialize(n, out);
  for (auto& p : value) {
    serialize(p.first, out);
    serialize(p.second, out);
  }
}

template <class K, class V>
uint32_t unserialize(ByteRange in, uint32_t pos, std::map<K, V>& value) {
  uint32_t n = 0;
  uint32_t p = pos + unserialize(in, pos, n);
  value.clear();
  K k;
  V v;
  for (uint32_t i = 0; i < n; i++) {
    p += unserialize(in, p, k);
    p += unserialize(in, p, v);
    value.emplace(k, v);
  }
  return p - pos;
}

template <class K, class V, class Out>
void serialize(const std::pair<K, V>& value, Out& out) {
  serialize(value.first, out);
  serialize(value.second, out);
}

template <class K, class V>
uint32_t unserialize(ByteRange in, uint32_t pos, std::pair<K, V>& value) {
  uint32_t p = pos;
  p += unserialize(in, p, value.first);
  p += unserialize(in, p, value.second);
  return p - pos;
}

/*
 * Macro for generate serialize and unserialize functions for struct type.
 * Usage:
 *
 *  namespace rdd {
 *
 *  struct A {
 *    int i;
 *    float j;
 *  };
 *  RDD_SERIALIZER(A, i, j)
 *
 *  }
 */

#define RDD_SERIALIZE_X(_a)   serialize(value._a, out);
#define RDD_UNSERIALIZE_X(_a) p += unserialize(in, p, value._a);

#define RDD_SERIALIZER(_type, ...)                                      \
template <class Out>                                                    \
inline void serialize(const _type& value, Out& out) {                   \
  RDD_APPLYXn(RDD_SERIALIZE_X, __VA_ARGS__)                             \
}                                                                       \
inline uint32_t unserialize(ByteRange in, uint32_t pos, _type& value) { \
  uint32_t p = pos;                                                     \
  RDD_APPLYXn(RDD_UNSERIALIZE_X, __VA_ARGS__)                           \
  return p - pos;                                                       \
}

} // namespace rdd
