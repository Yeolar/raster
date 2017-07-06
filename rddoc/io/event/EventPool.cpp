/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/io/event/EventPool.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

std::shared_ptr<Event> EventPool::get(const Peer& peer) {
  std::lock_guard<std::mutex> guard(lock_);
  auto& q = pool_[peer];
  if (!q.empty()) {
    std::shared_ptr<Event> event = q.front();
    q.pop_front();
    return event;
  }
  return nullptr;
}

bool EventPool::giveBack(const std::shared_ptr<Event>& event) {
  std::lock_guard<std::mutex> guard(lock_);
  auto& q = pool_[event->peer()];
  if (q.size() > 1000000) {
    RDDLOG(ERROR) << "too many events (>1000000)";
    return false;
  }
  q.push_back(event);
  return true;
}

size_t EventPool::count() const {
  std::lock_guard<std::mutex> guard(lock_);
  size_t n = 0;
  for (auto& kv : pool_) {
    n += kv.second.size();
  }
  return n;
}

}

