/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/EventPool.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

std::shared_ptr<Event> EventPool::get(const Peer& peer) {
  LockGuard guard(lock_);
  RDDMON_AVG("conn.pool-" + peer.str(), count());
  auto& q = pool_[peer];
  if (!q.empty()) {
    std::shared_ptr<Event> event = q.front();
    q.pop_front();
    return event;
  }
  return nullptr;
}

void EventPool::giveBack(const std::shared_ptr<Event>& event) {
  LockGuard guard(lock_);
  pool_[event->peer()].push_back(event);
}

size_t EventPool::count() const {
  size_t n = 0;
  for (auto& kv : pool_) {
    n += kv.second.size();
  }
  return n;
}

}

