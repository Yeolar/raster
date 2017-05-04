/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <limits>
#include <map>
#include <memory>
#include <queue>
#include "rddoc/util/MapUtil.h"
#include "rddoc/util/Time.h"

namespace rdd {

template <class T>
class TimedHeap {
public:
  TimedHeap() {}

  void push(Timeout<T> v) {
    if (!v.data) {
      return;
    }
    map_[v.data] = v;
    heap_.push(v);
  }

  Timeout<T> pop(uint64_t now) {
    while (!heap_.empty()) {
      auto timeout = heap_.top();
      if (get_default(map_, timeout.data) == timeout) {
        if (timeout.deadline <= now) {
          map_.erase(timeout.data);
          heap_.pop();
          return timeout;
        } else {
          break;
        }
      } else {
        heap_.pop();
      }
    }
    return Timeout<T>();
  }

  void erase(T* k) {
    if (!k) {
      return;
    }
    map_.erase(k);
  }

private:
  std::map<T*, Timeout<T>> map_;
  std::priority_queue<
    Timeout<T>, std::vector<Timeout<T>>, std::greater<Timeout<T>>> heap_;
};

}

