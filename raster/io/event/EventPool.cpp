/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/EventPool.h"

namespace rdd {

std::unique_ptr<Event> EventPool::get(const Peer& peer) {
  std::lock_guard<std::mutex> guard(lock_);
  auto& q = pool_[peer];
  if (!q.empty()) {
    auto event = std::move(q.front());
    q.pop_front();
    return event;
  }
  return nullptr;
}

bool EventPool::giveBack(std::unique_ptr<Event> event) {
  std::lock_guard<std::mutex> guard(lock_);
  auto& q = pool_[event->peer()];
  if (q.size() > 1000000) {
    RDDLOG(ERROR) << "too many events (>1000000)";
    return false;
  }
  q.push_back(std::move(event));
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

EventPool* EventPoolManager::getPool(int id) {
  std::lock_guard<std::mutex> guard(lock_);
  if (pool_.find(id) == pool_.end()) {
    pool_.emplace(id, make_unique<EventPool>());
  }
  return pool_[id].get();
}

} // namespace rdd
