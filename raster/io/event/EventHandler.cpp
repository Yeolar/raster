/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"
#include "raster/io/event/EventHandler.h"
#include "raster/net/Actor.h"

namespace rdd {

void EventHandler::onListen(Event* event) {
  assert(event->state() == Event::kListen);

  auto socket = event->socket()->accept();
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking())) {
    return;
  }
  if (Singleton<Actor>::get()->exceedConnectionLimit()) {
    RDDLOG(WARN) << "exceed connection capacity, drop request";
    return;
  }

  auto evnew = make_unique<Event>(event->channel(), std::move(socket));
  RDDLOG(V1) << *evnew << " accepted";
  evnew->setState(Event::kNext);
  loop_->dispatchEvent(evnew);
}

void EventHandler::onConnect(Event* event) {
  assert(event->state() == Event::kConnect);

  if (event->isConnectTimeout()) {
    event->setState(Event::kTimeout);
    RDDLOG(WARN) << *event << " remove connect timeout request: >"
      << event->timeoutOption().ctimeout;
    onTimeout(event);
    return;
  }
  int err = 1;
  event->socket()->getError(err);
  if (err != 0) {
    RDDLOG(ERROR) << *event << " connect: close for error: "
      << strerror(errno);
    event->setState(Event::kError);
    onError(event);
    return;
  }
  RDDLOG(V1) << *event << " connect: complete";
  event->setState(Event::kToWrite);
}

void EventHandler::onRead(Event* event) {
  event->setState(Event::kReading);

  int r = event->readData();
  switch (r) {
    case 1: {
      RDDLOG(V1) << *event << " read: complete";
      event->setState(Event::kReaded);
      onComplete(event);
      break;
    }
    case -1: {
      RDDLOG(ERROR) << *event << " read: close for error: "
        << strerror(errno);
      event->setState(Event::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isReadTimeout()) {
        event->setState(Event::kTimeout);
        RDDLOG(WARN) << *event << " remove read timeout request: >"
          << event->timeoutOption().rtimeout;
        onTimeout(event);
      } else {
        RDDLOG(V1) << *event << " read: again";
      }
      break;
    }
    case 0:
    case -3: {
      RDDLOG(V1) << *event << " read: peer is closed";
      closePeer(event);
      break;
    }
    default: break;
  }
}

void EventHandler::onWrite(Event* event) {
  event->setState(Event::kWriting);

  int r = event->writeData();
  switch (r) {
    case 1: {
      RDDLOG(V1) << *event << " write: complete";
      event->setState(Event::kWrited);
      onComplete(event);
      break;
    }
    case -1: {
      RDDLOG(ERROR) << *event << " write: close for error: "
        << strerror(errno);
      event->setState(Event::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isWriteTimeout()) {
        event->setState(Event::kTimeout);
        RDDLOG(WARN) << *event << " remove write timeout request: >"
          << event->timeoutOption().wtimeout;
        onTimeout(event);
      } else {
        RDDLOG(V1) << *event << " write: again";
      }
      break;
    }
    default: break;
  }
}

void EventHandler::onComplete(Event* event) {
  // for server: kReaded -> kWrited
  // for client: kWrited -> kReaded

  if (event->state() == Event::kReaded) {
    if (event->isReadTimeout()) {
      event->setState(Event::kTimeout);
      RDDLOG(WARN) << *event << " remove read timeout request: >"
        << event->timeoutOption().rtimeout;
      onTimeout(event);
      return;
    }

    auto ev = loop_->popEvent(event);

    // on result
    if (ev->socket()->isClient()) {
      RDDMON_CNT("conn.success-" + ev->label());
      RDDMON_AVG("conn.cost-" + ev->label(), ev->cost() / 1000);
    }
    if (!ev->isForward()) {
      Singleton<Actor>::get()->execute(ev);
    }
    return;
  }

  if (event->state() == Event::kWrited) {
    if (event->isWriteTimeout()) {
      event->setState(Event::kTimeout);
      RDDLOG(WARN) << *event << " remove write timeout request: >"
        << event->timeoutOption().wtimeout;
      onTimeout(event);
      return;
    }

    loop_->updateEvent(event, EPoll::kRead);

    // server: wait next; client: wait response
    if (event->socket()->isServer()) {
      RDDMON_CNT("conn.success-" + event->label());
      RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
      event->reset();
      event->setState(Event::kNext);
    } else {
      event->setState(Event::kToRead);
    }
    loop_->dispatchEvent(event);
    return;
  }
}

void EventHandler::onTimeout(Event *event) {
  assert(event->state() == Event::kTimeout);

  RDDMON_CNT("conn.timeout-" + event->label());
  closePeer(event);
}

void EventHandler::onError(Event* event) {
  assert(event->state() == Event::kError);

  RDDMON_CNT("conn.error-" + event->label());
  closePeer(event);
}

void EventHandler::closePeer(Event* event) {
  auto ev = loop_->popEvent(event);

  if (ev->socket()->isClient()) {
    ev->setState(Event::kFail);
    Singleton<Actor>::get()->execute(ev);
  }
}

} // namespace rdd
