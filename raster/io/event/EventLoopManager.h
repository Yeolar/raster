/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <set>
#include <stdexcept>
#include "raster/io/event/EventLoop.h"
#include "raster/util/ThreadUtil.h"

namespace rdd {

class EventLoopManager {
public:
  EventLoopManager() {}
  ~EventLoopManager() {}

  static EventLoopManager* get();

  EventLoop* getEventLoop() {
    auto info = localStore_.get();
    if (!info) {
      info = new EventLoopInfo();
      localStore_.reset(info);
      trackEventLoop(info->loop_);
    }
    return info->loop_;
  }

  void setEventLoop(EventLoop* loop, bool takeOwnership) {
    EventLoopInfo* info = localStore_.get();
    if (info) {
      throw std::runtime_error("EventLoopManager: cannot set a new EventLoop "
                               "for this thread when one already exists");
    }
    info = new EventLoopInfo(loop, takeOwnership);
    localStore_.reset(info);
    trackEventLoop(loop);
  }

  void clearEventLoop() {
    EventLoopInfo* info = localStore_.get();
    if (info) {
      untrackEventLoop(info->loop_);
      localStore_.reset(nullptr);
    }
  }

  template<typename F>
  void withEventLoopSet(const F& runnable) {
    std::lock_guard<std::mutex> guard(loopsLock_);
    const std::set<EventLoop*>& constSet = loops_;
    runnable(constSet);
  }

  NOCOPY(EventLoopManager);

private:
  struct EventLoopInfo {
    EventLoopInfo(EventLoop* loop, bool owned)
      : loop_(loop), owned_(owned) {}

    EventLoopInfo()
      : loop_(new EventLoop), owned_(true) {}

    ~EventLoopInfo() {
      if (owned_) {
        delete loop_;
      }
    }

    EventLoop* loop_;
    bool owned_;
  };

  void trackEventLoop(EventLoop* loop) {
    std::lock_guard<std::mutex> guard(loopsLock_);
    loops_.insert(loop);
  }

  void untrackEventLoop(EventLoop* loop) {
    std::lock_guard<std::mutex> guard(loopsLock_);
    loops_.erase(loop);
  }

  mutable ThreadLocalPtr<EventLoopInfo> localStore_;

  mutable std::set<EventLoop*> loops_;
  std::mutex loopsLock_;
};

} // namespace rdd
