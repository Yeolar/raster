/*
 * Copyright (C) 2017, Yeolar
 */

#include <atomic>

#include "raster/io/event/EventLoopManager.h"

namespace rdd {

std::atomic<EventLoopManager*> globalManager(nullptr);

EventLoopManager* EventLoopManager::get() {
  EventLoopManager* mgr = globalManager;
  if (mgr) {
    return mgr;
  }
  EventLoopManager* new_mgr = new EventLoopManager;
  bool exchanged = globalManager.compare_exchange_strong(mgr, new_mgr);
  if (!exchanged) {
    delete new_mgr;
    return mgr;
  } else {
    return new_mgr;
  }

}

} // namespace rdd
