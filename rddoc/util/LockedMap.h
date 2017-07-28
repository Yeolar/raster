/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <mutex>

namespace rdd {

template <class K, class V>
class LockedMap {
public:
  typedef typename std::map<K, V>::iterator iterator;
  typedef typename std::map<K, V>::value_type value_type;

  LockedMap() {}

  V& operator[](const K& k) {
    std::lock_guard<std::mutex> guard(lock_);
    return map_[k];
  }

  const V& operator[](const K& k) const {
    std::lock_guard<std::mutex> guard(lock_);
    return map_[k];
  }

  std::pair<V, bool> insert(const K& k, const V& v) {
    return insert(value_type(k, v));
  }

  std::pair<V, bool> insert(const value_type& v) {
    std::lock_guard<std::mutex> guard(lock_);
    std::pair<iterator, bool> r = map_.insert(v);
    if (r.second) {
      return std::make_pair(r.first->second, true);
    } else {
      return std::make_pair(V(), false);
    }
  }

  V erase(const K& k) {
    std::lock_guard<std::mutex> guard(lock_);
    iterator it = map_.find(k);
    if (it == map_.end()) {
      return V();
    }
    V r = it->second;
    map_.erase(it);
    return r;
  }

  const V& get(const K& k) const {
    std::lock_guard<std::mutex> guard(lock_);
    iterator it = map_.find(k);
    return it != map_.end() ? it->second : V();
  }

  bool contains(const K& k) const {
    std::lock_guard<std::mutex> guard(lock_);
    return map_.find(k) != map_.end();
  }

  size_t size() const {
    std::lock_guard<std::mutex> guard(lock_);
    return map_.size();
  }

  bool empty() const {
    std::lock_guard<std::mutex> guard(lock_);
    return map_.empty();
  }

  void clear() {
    std::lock_guard<std::mutex> guard(lock_);
    map_.clear();
  }

  void for_each(std::function<void(K& k, V& v)> func) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto& kv : map_) {
      func(kv.first, kv.second);
    }
  }

  void for_each(std::function<void(const K& k, const V& v)> func) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto& kv : map_) {
      func(kv.first, kv.second);
    }
  }

private:
  std::map<K, V> map_;
  mutable std::mutex lock_;
};

}

