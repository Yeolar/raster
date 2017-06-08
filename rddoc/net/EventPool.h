/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <deque>
#include <map>
#include <memory>
#include "rddoc/net/Event.h"
#include "rddoc/net/NetUtil.h"
#include "rddoc/util/Lock.h"
#include "rddoc/util/LockedMap.h"

namespace rdd {

class EventPool {
public:
  EventPool() {}

  std::shared_ptr<Event> get(const Peer& peer);

  void giveBack(const std::shared_ptr<Event>& event);

private:
  size_t count() const;

  std::map<Peer, std::deque<std::shared_ptr<Event>>> pool_;
  Lock lock_;
};

class EventPoolManager {
public:
  EventPoolManager() {}

  EventPool* getPool(int id) {
    return pool_[id].get();
  }

private:
  LockedMap<int, std::shared_ptr<EventPool>> pool_;
};

}

