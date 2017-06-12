/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>
#include "rddoc/io/Waker.h"
#include "rddoc/net/Event.h"
#include "rddoc/net/EventHandler.h"
#include "rddoc/net/Poll.h"
#include "rddoc/util/LockedDeq.h"
#include "rddoc/util/TimedHeap.h"

namespace rdd {

class Channel;

class EventLoop {
public:
  EventLoop(int poll_size = Poll::MAX_EVENTS,
            int poll_timeout = 1000/* 1s */)
    : poll_(poll_size)
    , timeout_(poll_timeout)
    , running_(false)
    , handler_(this) {
    poll_.add(waker_.fd(), EPOLLIN, new Event(&waker_));
  }

  void listen(const std::shared_ptr<Channel>& channel, int backlog = 64);

  void start();
  void stop();

  void addEvent(Event* event);
  void addCallback(const VoidCallback& callback);

  friend class EventHandler;

private:
  void dispatchEvent(Event* event);
  void addListenEvent(Event* event);
  void addReadEvent(Event* event);
  void addWriteEvent(Event* event);
  void removeEvent(Event* event);
  void restartEvent(Event* event);

  void checkTimeoutEvent();

  Poll poll_;
  int timeout_;
  std::atomic<bool> running_;

  std::vector<int> listen_fds_;
  Waker waker_;

  EventHandler handler_;

  std::vector<Event*> events_;
  Lock event_lock_;
  std::vector<VoidCallback> callbacks_;
  Lock callback_lock_;

  TimedHeap<Event> deadline_heap_;
};

}

