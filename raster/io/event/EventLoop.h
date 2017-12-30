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

  void listen(const std::shared_ptr<Channel>& channel, int backlog = 64);

  bool isRunning() const {
    return loopThread_ != 0;
  }

  bool inLoopThread() const {
    return loopThread_ == 0 || pthread_equal(loopThread_, pthread_self());
  }

  bool inRunningLoopThread() const {
    return pthread_equal(loopThread_, pthread_self());
  }

  void waitUntilRunning() {
    while (!isRunning()) {
      std::this_thread::yield();
    }
  }

  void loop() { loopBody(false); }
  void loopOnce() { loopBody(true); }
  void stop();

  void addEvent(std::unique_ptr<Event> event);
  void addCallback(VoidFunc&& callback);

 private:
  void loopBody(bool once);

  void dispatchEvent(std::unique_ptr<Event> event);
  void dispatchEvent(Event* event);

  void updateEvent(Event* event, uint32_t events);

  void restartEvent(Event* event);

  std::unique_ptr<Event> popEvent(Event* event);

  void checkTimeoutEvents();

  EPoll poll_;
  int timeout_;

  std::atomic<bool> stop_;
  std::atomic<pthread_t> loopThread_;

  std::vector<int> listenFds_;
  Waker waker_;
  std::map<int, std::unique_ptr<Event>> fdEvents_;
  EventHandler handler_;

  std::vector<std::unique_ptr<Event>> events_;
  std::mutex eventsLock_;
  std::vector<VoidFunc> callbacks_;
  std::mutex callbacksLock_;

  TimedHeap<Event> deadlineHeap_;
};

} // namespace rdd
