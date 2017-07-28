/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/io/event/EventHandler.h"
#include "rddoc/net/Actor.h"
#include "rddoc/net/Protocol.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

void EventHandler::handle(Event* event, uint32_t etype) {
  RDD_EVLOG(V2, event) << "on event, type=" << etype;
  switch (event->type()) {
    case Event::LISTEN:
      onListen(event); break;
    case Event::CONNECT:
      onConnect(event); break;
    case Event::NEXT:
      loop_->restartEvent(event);
    case Event::TOREAD:
    case Event::READING:
      onRead(event); break;
    case Event::TOWRITE:
    case Event::WRITING:
      onWrite(event); break;
    case Event::TIMEOUT:
      onTimeout(event); break;
    default:
      RDD_EVLOG(ERROR, event) << "error event, type=" << etype;
      closePeer(event);
      break;
  }
}

void EventHandler::onListen(Event* event) {
  assert(event->type() == Event::LISTEN);
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
  RDD_EVLOG(V1, evnew) << "accepted";
  if (evnew->isConnectTimeout()) {
    evnew->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, evnew) << "remove connect timeout request: >"
      << evnew->timeoutOption().ctimeout;
    onTimeout(evnew);
    return;
  }
  evnew->setType(Event::NEXT);
  loop_->dispatchEvent(evnew);
}

void EventHandler::onConnect(Event* event) {
  assert(event->type() == Event::CONNECT);
  if (event->isConnectTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove connect timeout request: >"
      << event->timeoutOption().ctimeout;
    onTimeout(event);
    return;
  }
  int err = 1;
  event->socket()->getError(err);
  if (err != 0) {
    RDD_EVLOG(ERROR, event) << "connect: close for error: " << strerror(errno);
    event->setType(Event::ERROR);
    onError(event);
    return;
  }
  RDD_EVLOG(V1, event) << "connect: complete";
  event->setType(Event::TOWRITE);
}

void EventHandler::onRead(Event* event) {
  event->setType(Event::READING);
  int r = event->readData();
  switch (r) {
    case -1:
    {
      if (event->type() == Event::TIMEOUT) {
        RDD_EVTLOG(WARN, event) << "remove read timeout request: >"
          << event->timeoutOption().rtimeout;
        onTimeout(event);
      } else {
        RDD_EVLOG(ERROR, event) << "read: close for error: "
          << strerror(errno);
        event->setType(Event::ERROR);
        onError(event);
      }
      break;
    }
    case 0:
    {
      RDD_EVLOG(V1, event) << "read: complete";
      event->setType(Event::READED);
      onComplete(event);
      break;
    }
    case 1:
    {
      RDD_EVLOG(V1, event) << "read: again";
      break;
    }
    case 2:
    {
      RDD_EVLOG(V1, event) << "read: peer is closed";
      closePeer(event);
      break;
    }
    default: break;
  }
}

void EventHandler::onWrite(Event* event) {
  event->setType(Event::WRITING);
  int r = event->writeData();
  switch (r) {
    case -1:
    {
      if (event->type() == Event::TIMEOUT) {
        RDD_EVTLOG(WARN, event) << "remove write timeout request: >"
          << event->timeoutOption().wtimeout;
        onTimeout(event);
      } else {
        RDD_EVLOG(ERROR, event) << "write: close for error: "
          << strerror(errno);
        event->setType(Event::ERROR);
        onError(event);
      }
      break;
    }
    case 0:
    {
      RDD_EVLOG(V1, event) << "write: complete";
      event->setType(Event::WRITED);
      onComplete(event);
      break;
    }
    case 1:
    {
      RDD_EVLOG(V1, event) << "write: again";
      break;
    }
    default: break;
  }
}

void EventHandler::onComplete(Event* event) {
  if (event->type() == Event::READED && event->isReadTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove read timeout request: >"
      << event->timeoutOption().rtimeout;
    onTimeout(event);
    return;
  }
  if (event->type() == Event::WRITED && event->isWriteTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove write timeout request: >"
      << event->timeoutOption().wtimeout;
    onTimeout(event);
    return;
  }
  loop_->removeEvent(event);

  // for server: READED -> WRITED
  // for client: WRITED -> READED

  switch (event->type()) {
    // on result
    case Event::READED:
    {
      if (event->role() == Socket::CLIENT) {
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
    case Event::WRITED:
    {
      if (event->role() == Socket::SERVER) {
        RDDMON_CNT("conn.success-" + event->label());
        RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
        event->reset();
        event->setType(Event::NEXT);
      } else {
        event->setType(Event::TOREAD);
      }
      loop_->dispatchEvent(event);
      break;
    }
    default: break;
  }
}

void EventHandler::onTimeout(Event *event) {
  assert(event->type() == Event::TIMEOUT);
  RDDMON_CNT("conn.timeout-" + event->label());
  closePeer(event);
}

void EventHandler::onError(Event* event) {
  assert(event->type() == Event::ERROR);
  RDDMON_CNT("conn.error-" + event->label());
  closePeer(event);
}

void EventHandler::closePeer(Event* event) {
  loop_->removeEvent(event);
  if (event->role() == Socket::CLIENT) {
    event->setType(Event::FAIL);
    Singleton<Actor>::get()->execute(event);
  } else {
    delete event;
  }
}

} // namespace rdd
