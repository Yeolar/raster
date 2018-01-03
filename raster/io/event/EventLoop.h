/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <list>
#include <map>
#include <vector>

#include "raster/io/Waker.h"
#include "raster/io/event/EPoll.h"
#include "raster/io/event/Event.h"
#include "raster/io/event/EventHandler.h"
#include "raster/util/TimedHeap.h"

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
  void addCallback(VoidFunc&& callback);

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
  Waker waker_;
  std::map<int, Event*> fdEvents_;
  EventHandler handler_;

  std::vector<Event*> events_;
  std::mutex eventsLock_;
  std::vector<VoidFunc> callbacks_;
  std::mutex callbacksLock_;

  TimedHeap<Event> deadlineHeap_;
};

} // namespace rdd
