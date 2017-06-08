/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>
#include "rddoc/util/Lock.h"

namespace rdd {

template <class K, class V>
class LockedMap {
public:
  LockedMap() {}

  void insert(const K& k, const V& v) {
    LockGuard guard(lock_);
    map_.emplace(k, v);
  }

  void update(const K& k, const V& v) {
    LockGuard guard(lock_);
    map_[k] = v;
  }

  void erase(const K& k) {
    LockGuard guard(lock_);
    map_.erase(k);
  }

  bool get(const K& k, V& v) const {
    LockGuard guard(lock_);
    auto it = map_.find(k);
    if (it != map_.end()) {
      v = it->second;
      return true;
    }
    return false;
  }

  V get(const K& k) const {
    V v = V();
    get(k, v);
    return v;
  }

  V operator[](const K& k) {
    LockGuard guard(lock_);
    return map_[k];
  }

  size_t size() const {
    LockGuard guard(lock_);
    return map_.size();
  }

  std::vector<K> keys() const {
    std::vector<K> v;
    LockGuard guard(lock_);
    for (auto& kv : map_) {
      v.push_back(kv.first);
    }
    return v;
  }

  std::vector<V> values() const {
    std::vector<V> v;
    LockGuard guard(lock_);
    for (auto& kv : map_) {
      v.push_back(kv.second);
    }
    return v;
  }

private:
  std::map<K, V> map_;
  mutable Lock lock_;
};

}

