/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>
#include "rddoc/io/Waker.h"
#include "rddoc/net/Event.h"
#include "rddoc/util/LockedDeq.h"

namespace rdd {

class EventLoop;

class EventHandler {
public:
  EventHandler(EventLoop* loop) : loop_(loop) {}

  void handle(Event* event, uint32_t etype);

  friend class EventLoop;

private:
  void onListen(Event* event);
  void onConnect(Event* event);
  void onRead(Event* event);
  void onWrite(Event* event);
  void onComplete(Event* event);
  void onTimeout(Event* event);
  void onError(Event* event);

  void closePeer(Event* event);

  EventLoop* loop_;
};

}

