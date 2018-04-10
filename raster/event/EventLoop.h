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

#include <list>
#include <map>
#include <vector>

#include "accelerator/io/Waker.h"
#include "raster/event/EPoll.h"
#include "raster/event/Event.h"
#include "raster/event/EventHandler.h"
#include "accelerator/TimedHeap.h"

namespace rdd {

class Channel;

class EventLoop {
 public:
  EventLoop(int pollSize = EPoll::kMaxEvents,
            int pollTimeout = 1000/* 1s */);

  ~EventLoop() {}

  void loop();
  void loopOnce();

  void stop();

  void addEvent(Event* event);
  void addCallback(acc::VoidFunc&& callback);

  friend class EventHandler;

 private:
  void loopBody(bool once = false);

  void dispatchEvent(Event* event);
  void updateEvent(Event* event, uint32_t events);
  void restartEvent(Event* event);

  void pushEvent(Event* event);
  void popEvent(Event* event);

  void checkTimeoutEvents();

  EPoll poll_;
  int timeout_;

  std::atomic<bool> stop_;
  std::atomic<std::thread::id> loopThread_;

  std::vector<int> listenFds_;
  acc::Waker waker_;
  std::map<int, Event*> fdEvents_;
  EventHandler handler_;

  std::vector<Event*> events_;
  std::mutex eventsLock_;
  std::vector<acc::VoidFunc> callbacks_;
  std::mutex callbacksLock_;

  acc::TimedHeap<Event> deadlineHeap_;
};

} // namespace rdd
