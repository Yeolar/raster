/*
 * Copyright (C) 2018, Yeolar
 */

#pragma once

#include "raster/coroutine/FiberHub.h"
#include "raster/io/event/EventLoop.h"
#include "raster/net/Group.h"

namespace rdd {

struct ForwardTarget {
  int port{0};
  Peer fpeer;
  int flow{0};
};

class NetHub : public FiberHub {
 public:
  virtual EventLoop* getEventLoop() = 0;

  void execute(Event* event);

  void addEvent(Event* event);
  void forwardEvent(Event* event, const Peer& peer);

  bool waitGroup(const std::vector<Event*>& events);

  void setForwarding(bool forward);
  void addForwardTarget(ForwardTarget&& t);

 private:
  Group group_;
  bool forwarding_;
  std::vector<ForwardTarget> forwards_;
};

} // namespace rdd
