/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "raster/io/event/Event.h"
#include "raster/net/NetUtil.h"

namespace rdd {

class EventPool {
public:
  EventPool() {}

  std::shared_ptr<Event> get(const Peer& peer);

  bool giveBack(const std::shared_ptr<Event>& event);

  size_t count() const;

private:
  std::unordered_map<Peer, std::deque<std::shared_ptr<Event>>> pool_;
  mutable std::mutex lock_;
};

class EventPoolManager {
public:
  EventPoolManager() {}

  EventPool* getPool(int id) {
    std::lock_guard<std::mutex> guard(lock_);
    if (pool_.find(id) == pool_.end()) {
      pool_.emplace(id, std::make_shared<EventPool>());
    }
    return pool_[id].get();
  }

private:
  std::unordered_map<int, std::shared_ptr<EventPool>> pool_;
  mutable std::mutex lock_;
};

} // namespace rdd
