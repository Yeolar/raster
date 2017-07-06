/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <list>
#include <vector>
#include "rddoc/io/Waker.h"
#include "rddoc/io/event/Event.h"
#include "rddoc/io/event/EventHandler.h"
#include "rddoc/io/event/Poll.h"
#include "rddoc/util/Lock.h"
#include "rddoc/util/TimedHeap.h"

namespace rdd {

class Channel;

class EventLoop {
public:
  EventLoop(int pollSize = Poll::MAX_EVENTS,
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
      sched_yield();
    }
  }

  void loop() { loopBody(false); }
  void loopOnce() { loopBody(true); }
  void stop();

  void addEvent(Event* event);
  void addCallback(const VoidFunc& callback);

  friend class EventHandler;

private:
  void loopBody(bool once);

  void dispatchEvent(Event* event);
  void addListenEvent(Event* event);
  void addReadEvent(Event* event);
  void addWriteEvent(Event* event);
  void removeEvent(Event* event);
  void restartEvent(Event* event);

  void checkTimeoutEvent();

  Poll poll_;
  int timeout_;

  std::atomic<bool> stop_;
  std::atomic<pthread_t> loopThread_;

  std::vector<int> listenFDs_;
  Waker waker_;
  EventHandler handler_;

  std::vector<Event*> events_;
  SpinLock eventsLock_;
  std::vector<VoidFunc> callbacks_;
  SpinLock callbacksLock_;

  TimedHeap<Event> deadlineHeap_;
};

}

