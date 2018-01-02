/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/EventLoopManager.h"

#include <atomic>

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

EventLoop* EventLoopManager::getEventLoop() {
  auto info = localStore_.get();
  if (!info) {
    info = new EventLoopInfo();
    localStore_.reset(info);
    trackEventLoop(info->loop_);
  }
  return info->loop_;
}

void EventLoopManager::setEventLoop(EventLoop* loop, bool takeOwnership) {
  EventLoopInfo* info = localStore_.get();
  if (info) {
    throw std::runtime_error("EventLoopManager: cannot set a new EventLoop "
                             "for this thread when one already exists");
  }
  info = new EventLoopInfo(loop, takeOwnership);
  localStore_.reset(info);
  trackEventLoop(loop);
}

void EventLoopManager::clearEventLoop() {
  EventLoopInfo* info = localStore_.get();
  if (info) {
    untrackEventLoop(info->loop_);
    localStore_.reset(nullptr);
  }
}

} // namespace rdd
