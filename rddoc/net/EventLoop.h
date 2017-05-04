/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>
#include "rddoc/io/Waker.h"
#include "rddoc/net/Event.h"
#include "rddoc/net/Poll.h"
#include "rddoc/util/LockedDeq.h"
#include "rddoc/util/TimedHeap.h"

namespace rdd {

class Actor;
class Channel;

class EventLoop {
public:
  EventLoop(Actor* actor,
            int poll_size = Poll::MAX_EVENTS,
            int poll_timeout = 1000/* 1s */)
    : actor_(actor)
    , poll_(poll_size)
    , timeout_(poll_timeout)
    , running_(false) {
    poll_.add(waker_.fd(), EPOLLIN, new Event(&waker_));
  }

  void listen(const std::shared_ptr<Channel>& channel, int backlog = 64);

  void start();
  void stop();

  void addEvent(Event* event);
  void addCallback(const PtrCallback& callback, void* ptr);

private:
  void dispatchEvent(Event* event);
  void addListenEvent(Event* event);
  void addReadEvent(Event* event);
  void addWriteEvent(Event* event);
  void removeEvent(Event* event);

  void handleEvent(int i);

  void handleListen(Event* event);
  void handleConnect(Event* event);
  void handleRead(Event* event);
  void handleWrite(Event* event);
  void handleComplete(Event* event);
  void handleTimeout(Event* event);
  void handleError(Event* event);

  void checkTimeoutEvent();

  void closePeer(Event* event);

  Actor* actor_;
  Poll poll_;
  int timeout_;
  std::atomic<bool> running_;

  std::vector<int> listen_fds_;
  Waker waker_;

  std::vector<Event*> events_;
  Lock event_lock_;
  std::vector<std::pair<PtrCallback, void*>> callbacks_;
  Lock callback_lock_;

  TimedHeap<Event> deadline_heap_;
};

}

