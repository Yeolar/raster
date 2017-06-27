/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <deque>
#include <map>
#include <memory>
#include "rddoc/io/event/Event.h"
#include "rddoc/net/NetUtil.h"
#include "rddoc/util/Lock.h"

namespace rdd {

class EventPool {
public:
  EventPool() {}

  std::shared_ptr<Event> get(const Peer& peer);

  bool giveBack(const std::shared_ptr<Event>& event);

  size_t count() const;

private:
  std::map<Peer, std::deque<std::shared_ptr<Event>>> pool_;
  mutable Lock lock_;
};

class EventPoolManager {
public:
  EventPoolManager() {}

  EventPool* getPool(int id) {
    LockGuard guard(lock_);
    if (pool_.find(id) == pool_.end()) {
      pool_.emplace(id, std::make_shared<EventPool>());
    }
    return pool_[id].get();
  }

private:
  std::map<int, std::shared_ptr<EventPool>> pool_;
  mutable Lock lock_;
};

}

