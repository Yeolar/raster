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

#include "raster/event/EventLoopManager.h"

#include <atomic>

namespace raster {

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

} // namespace raster
