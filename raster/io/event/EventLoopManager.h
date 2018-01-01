/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <set>
#include <stdexcept>

#include "raster/io/event/EventLoop.h"
#include "raster/thread/ThreadUtil.h"

namespace rdd {

class EventLoopManager {
 public:
  EventLoopManager() {}
  ~EventLoopManager() {}

  static EventLoopManager* get();

  EventLoop* getEventLoop();

  void setEventLoop(EventLoop* loop, bool takeOwnership);

  void clearEventLoop();

  template<typename F>
  void withEventLoopSet(F&& runnable) {
    std::lock_guard<std::mutex> guard(loopsLock_);
    const std::set<EventLoop*>& constSet = loops_;
    runnable(constSet);
  }

  EventLoopManager(const EventLoopManager&) = delete;
  EventLoopManager& operator=(const EventLoopManager&) = delete;

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
