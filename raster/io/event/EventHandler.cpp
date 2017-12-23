/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"
#include "raster/io/event/EventHandler.h"
#include "raster/net/Actor.h"

namespace rdd {

void EventHandler::handle(Event* event, uint32_t etype) {
  RDDLOG(V2) << *event << " on event, type=" << etype;
  switch (event->state()) {
    case Event::kListen:
      onListen(event); break;
    case Event::kConnect:
      onConnect(event); break;
    case Event::kNext:
      loop_->restartEvent(event);
    case Event::kToRead:
    case Event::kReading:
      onRead(event); break;
    case Event::kToWrite:
    case Event::kWriting:
      onWrite(event); break;
    case Event::kTimeout:
      onTimeout(event); break;
    default:
      RDDLOG(ERROR) << *event << " error event, type=" << etype;
      closePeer(event);
      break;
  }
}

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
  Event *evnew = new Event(event->channel(), socket);
  if (!evnew) {
    RDDLOG(ERROR) << "create event failed";
    return;
  }
  RDDLOG(V1) << *evnew << " accepted";
  if (evnew->isConnectTimeout()) {
    evnew->setState(Event::kTimeout);
    RDDLOG(WARN) << *evnew << " remove connect timeout request: >"
      << evnew->timeoutOption().ctimeout;
    onTimeout(evnew);
    return;
  }
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
    RDDLOG(ERROR) << *event << " connect: close for error: " << strerror(errno);
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
  if (event->state() == Event::kReaded && event->isReadTimeout()) {
    event->setState(Event::kTimeout);
    RDDLOG(WARN) << *event << " remove read timeout request: >"
      << event->timeoutOption().rtimeout;
    onTimeout(event);
    return;
  }
  if (event->state() == Event::kWrited && event->isWriteTimeout()) {
    event->setState(Event::kTimeout);
    RDDLOG(WARN) << *event << " remove write timeout request: >"
      << event->timeoutOption().wtimeout;
    onTimeout(event);
    return;
  }
  loop_->removeEvent(event);

  // for server: kReaded -> kWrited
  // for client: kWrited -> kReaded

  switch (event->state()) {
    // on result
    case Event::kReaded:
    {
      if (event->socket()->isClient()) {
        RDDMON_CNT("conn.success-" + event->label());
        RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
        if (event->isForward()) {
          delete event;
        }
      }
      Singleton<Actor>::get()->execute(event);
      break;
    }
    // server: wait next; client: wait response
    case Event::kWrited:
    {
      if (event->socket()->isServer()) {
        RDDMON_CNT("conn.success-" + event->label());
        RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
        event->reset();
        event->setState(Event::kNext);
      } else {
        event->setState(Event::kToRead);
      }
      loop_->dispatchEvent(event);
      break;
    }
    default: break;
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
  loop_->removeEvent(event);
  if (event->socket()->isClient()) {
    event->setState(Event::kFail);
    Singleton<Actor>::get()->execute(event);
  } else {
    delete event;
  }
}

} // namespace rdd
