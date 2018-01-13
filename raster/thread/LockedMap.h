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

#include "raster/thread/Synchronized.h"

namespace rdd {

template <class K, class V>
class LockedMap {
 public:
  typedef typename std::map<K, V>::key_type key_type;
  typedef typename std::map<K, V>::mapped_type mapped_type;
  typedef typename std::map<K, V>::value_type value_type;

  LockedMap() {}

  mapped_type operator[](const key_type& key) {
    return map_.wlock()->operator[](key);
  }

  mapped_type operator[](key_type&& key) {
    return map_.wlock()->operator[](std::move(key));
  }

  bool insert(const value_type& value) {
    return map_.wlock()->insert(value).second;
  }

  template <class... Args>
  bool emplace(Args&&... args) {
    return map_.wlock()->emplace(std::forward<Args>(args)...).second;
  }

  void update(const value_type& value) {
    map_.wlock()->operator[](value.first) = value.second;
  }

  size_t erase(const key_type& key) {
    return map_.wlock()->erase(key);
  }

  mapped_type erase_get(const key_type& key) {
    auto wlockedMap = map_.wlock();
    mapped_type value = wlockedMap->operator[](key);
    wlockedMap->erase(key);
    return std::move(value);
  }

  void clear() {
    map_.wlock()->clear();
  }

  size_t size() const {
    return map_.rlock()->size();
  }

  bool empty() const {
    return map_.rlock()->empty();
  }

  size_t count(const key_type& key) const {
    return map_.rlock()->count(key);
  }

  template <typename F>
  void for_each(F&& func) {
    auto wlockedMap = map_.wlock();
    for (auto& kv : *wlockedMap) {
      func(kv.first, kv.second);
    }
  }

  template <typename F>
  void for_each(F&& func) const {
    auto rlockedMap = map_.rlock();
    for (const auto& kv : *rlockedMap) {
      func(kv.first, kv.second);
    }
  }

 private:
  Synchronized<std::map<K, V>> map_;
};

} // namespace rdd
