/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <mutex>

#include "raster/util/RWLock.h"

namespace rdd {

template <class K, class V>
class LockedMap {
 public:
  typedef typename std::map<K, V>::iterator iterator;
  typedef typename std::map<K, V>::value_type value_type;

  LockedMap() {}

  V& operator[](const K& k) {
    WLockGuard guard(lock_);
    return map_[k];
  }

  const V& operator[](const K& k) const {
    RLockGuard guard(lock_);
    return map_[k];
  }

  std::pair<V, bool> insert(const K& k, const V& v) {
    return insert(value_type(k, v));
  }

  std::pair<V, bool> insert(const value_type& v) {
    WLockGuard guard(lock_);
    std::pair<iterator, bool> r = map_.insert(v);
    if (r.second) {
      return std::make_pair(r.first->second, true);
    } else {
      return std::make_pair(V(), false);
    }
  }

  V erase(const K& k) {
    WLockGuard guard(lock_);
    iterator it = map_.find(k);
    if (it == map_.end()) {
      return V();
    }
    V r = it->second;
    map_.erase(it);
    return r;
  }

  bool contains(const K& k) const {
    RLockGuard guard(lock_);
    return map_.find(k) != map_.end();
  }

  size_t size() const {
    RLockGuard guard(lock_);
    return map_.size();
  }

  bool empty() const {
    RLockGuard guard(lock_);
    return map_.empty();
  }

  void clear() {
    WLockGuard guard(lock_);
    map_.clear();
  }

  void for_each(std::function<void(K& k, V& v)> func) {
    WLockGuard guard(lock_);
    for (auto& kv : map_) {
      func(kv.first, kv.second);
    }
  }

  void for_each(std::function<void(const K& k, const V& v)> func) const {
    RLockGuard guard(lock_);
    for (auto& kv : map_) {
      func(kv.first, kv.second);
    }
  }

 private:
  std::map<K, V> map_;
  mutable RWLock lock_;
};

} // namespace rdd
