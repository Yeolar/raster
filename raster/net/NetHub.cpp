/*
 * Copyright (C) 2018, Yeolar
 */

#include "raster/net/NetHub.h"

#include "raster/io/event/EventTask.h"
#include "raster/net/Channel.h"

namespace rdd {

void NetHub::execute(Event* event) {
  if (event->task()) {
    size_t i = event->group();
    event->setGroup(0);
    if (i == 0 || group_.finish(i)) {
      FiberHub::execute(event->task()->fiber, event->channel()->id());
    }
  }
  int poolId = event->channel()->id();
  auto task = make_unique<EventTask>(event);
  task->scheduleCallback = [&]() { addEvent(event); };
  FiberHub::execute(std::move(task), poolId);
}

void NetHub::addEvent(Event* event) {
  if (forwarding_ && event->socket()->isClient()) {
    for (auto& f : forwards_) {
      if (f.port == event->channel()->id() && f.flow > rand() % 100) {
        forwardEvent(event, f.fpeer);
      }
    }
  }
  getEventLoop()->addEvent(event);
}

void NetHub::forwardEvent(Event* event, const Peer& peer) {
  auto socket = Socket::createAsyncSocket();
  if (!socket ||
      !socket->connect(peer)) {
    return;
  }
  auto evcopy = new Event(event->channel(), std::move(socket));
  evcopy->transport()->clone(event->transport());
  evcopy->setForward();
  evcopy->setState(Event::kWrited);
  getEventLoop()->addEvent(evcopy);
}

bool NetHub::waitGroup(const std::vector<Event*>& events) {
  for (auto& event : events) {
    if (event->group() != 0) {
      RDDLOG(WARN) << "create group on grouping Event, giveup";
      return false;
    }
  }
  size_t i = group_.create(events.size());
  for (auto& event : events) {
    event->setGroup(i);
  }
  return true;
}

void NetHub::setForwarding(bool forward) {
  forwarding_ = forward;
}

void NetHub::addForwardTarget(ForwardTarget&& t) {
  forwards_.push_back(std::move(t));
}

} // namespace rdd
