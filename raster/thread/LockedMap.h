/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>

#include "raster/thread/Synchronized.h"

namespace rdd {

template <class K, class V>
class LockedMap {
 public:
  LockedMap() {}

  V operator[](const K& key) {
    return map_.wlock()->operator[](key);
  }

  V operator[](K&& key) {
    return map_.wlock()->operator[](std::move(key));
  }

  bool insert(const V& value) {
    return map_.wlock()->insert(value).second;
  }

  template <class... Args>
  bool emplace(Args&&... args) {
    return map_.wlock()->emplace(std::forward<Args>(args)...).second;
  }

  size_t erase(const K& key) {
    return map_.wlock()->erase(key);
  }

  void clear() {
    map_.wlock()->clear();
  }

  size_t size() const {
    return map_.rlock()->size();
  }

  size_t count(const K& key) const {
    return map_.rlock(key)->count();
  }

  bool empty() const {
    return map_.rlock()->empty();
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
