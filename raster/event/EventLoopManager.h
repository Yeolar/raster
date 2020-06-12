/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <mutex>
#include <set>
#include <stdexcept>

#include <accelerator/thread/ThreadLocal.h>

#include "raster/event/EventLoop.h"

namespace raster {

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

  mutable acc::ThreadLocalPtr<EventLoopInfo> localStore_;

  mutable std::set<EventLoop*> loops_;
  std::mutex loopsLock_;
};

} // namespace raster
