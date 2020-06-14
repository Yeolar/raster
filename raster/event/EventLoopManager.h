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

#include <accelerator/thread/ThreadLocal.h>

#include "raster/event/EventLoop.h"

namespace raster {

class EventLoopManager {
 public:
  static EventLoopManager* getInstance();

  EventLoop* getEventLoop();

  void clearEventLoop();

  EventLoopManager(const EventLoopManager&) = delete;
  EventLoopManager& operator=(const EventLoopManager&) = delete;

 private:
  EventLoopManager() = default;

  acc::ThreadLocal<EventLoop> localStore_;
};

} // namespace raster
