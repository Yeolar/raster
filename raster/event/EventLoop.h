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

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include <accelerator/Waker.h>

#include "raster/event/EventBase.h"
#include "raster/event/EventHandlerBase.h"
#include "raster/event/Function.h"
#include "raster/event/Poll.h"
#include "raster/event/TimedHeap.h"

namespace raster {

class EventLoop {
 public:
  EventLoop(int pollSize = Poll::kMaxEvents,
            int pollTimeout = 1000/* 1s */);

  ~EventLoop() {}

  void registerHandler(std::unique_ptr<EventHandlerBase> handler);

  void loop();
  void loopOnce();

  void stop();

  void addEvent(EventBase* event);
  void addCallback(VoidFunc&& callback);

  void pushEvent(EventBase* event);
  void popEvent(EventBase* event);
  void updateEvent(EventBase* event, uint32_t events);
  void dispatchEvent(EventBase* event);

 private:
  void loopBody(bool once = false);

  void restartEvent(EventBase* event);

  void checkTimeoutEvents();

  std::unique_ptr<Poll> poll_;
  int timeout_;

  std::atomic<bool> stop_;
  std::atomic<std::thread::id> loopThread_;

  std::vector<int> listenFds_;
  acc::Waker waker_;
  std::map<int, EventBase*> fdEvents_;
  std::unique_ptr<EventHandlerBase> handler_;

  std::vector<EventBase*> events_;
  std::mutex eventsLock_;
  std::vector<VoidFunc> callbacks_;
  std::mutex callbacksLock_;

  TimedHeap<EventBase> deadlineHeap_;
};

} // namespace raster
