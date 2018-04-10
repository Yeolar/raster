/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/event/EventHandler.h"

#include "raster/framework/Monitor.h"
#include "raster/event/EventLoop.h"

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
  if (Socket::count() >= FLAGS_net_conn_limit) {
    ACCLOG(WARN) << "exceed connection capacity, drop request";
    return;
  }

  auto evnew = new Event(event->channel(), std::move(socket));
  evnew->copyCallbacks(*event);
  ACCLOG(V1) << *evnew << " accepted";
  evnew->setState(Event::kNext);
  ACCLOG(V2) << *evnew << " add event";
  loop_->pushEvent(evnew);
  loop_->dispatchEvent(evnew);
}

void EventHandler::onConnect(Event* event) {
  assert(event->state() == Event::kConnect);

  if (event->isConnectTimeout()) {
    event->setState(Event::kTimeout);
    ACCLOG(WARN) << *event << " remove connect timeout request: >"
      << event->timeoutOption().ctimeout;
    onTimeout(event);
    return;
  }
  int err = 1;
  event->socket()->getError(err);
  if (err != 0) {
    ACCLOG(ERROR) << *event << " connect: close for error: "
      << strerror(errno);
    event->setState(Event::kError);
    onError(event);
    return;
  }
  ACCLOG(V1) << *event << " connect: complete";
  event->setState(Event::kToWrite);
}

void EventHandler::onRead(Event* event) {
  event->setState(Event::kReading);

  int r = event->readData();
  switch (r) {
    case 1: {
      ACCLOG(V1) << *event << " read: complete";
      event->setState(Event::kReaded);
      onComplete(event);
      break;
    }
    case -1: {
      ACCLOG(ERROR) << *event << " read: close for error: "
        << strerror(errno);
      event->setState(Event::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isReadTimeout()) {
        event->setState(Event::kTimeout);
        ACCLOG(WARN) << *event << " remove read timeout request: >"
          << event->timeoutOption().rtimeout;
        onTimeout(event);
      } else {
        ACCLOG(V1) << *event << " read: again";
      }
      break;
    }
    case 0:
    case -3: {
      ACCLOG(V1) << *event << " read: peer is closed";
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
      ACCLOG(V1) << *event << " write: complete";
      event->setState(Event::kWrited);
      onComplete(event);
      break;
    }
    case -1: {
      ACCLOG(ERROR) << *event << " write: close for error: "
        << strerror(errno);
      event->setState(Event::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isWriteTimeout()) {
        event->setState(Event::kTimeout);
        ACCLOG(WARN) << *event << " remove write timeout request: >"
          << event->timeoutOption().wtimeout;
        onTimeout(event);
      } else {
        ACCLOG(V1) << *event << " write: again";
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
      ACCLOG(WARN) << *event << " remove read timeout request: >"
        << event->timeoutOption().rtimeout;
      onTimeout(event);
      return;
    }

    loop_->popEvent(event);

    // on result
    if (event->socket()->isClient()) {
      RDDMON_CNT("conn.success-" + event->label());
      RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
    }
    if (!event->isForward()) {
      event->callbackOnComplete();  // execute
    }
    return;
  }

  if (event->state() == Event::kWrited) {
    if (event->isWriteTimeout()) {
      event->setState(Event::kTimeout);
      ACCLOG(WARN) << *event << " remove write timeout request: >"
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
  loop_->popEvent(event);

  if (event->socket()->isClient()) {
    event->setState(Event::kFail);
    event->callbackOnClose();  // execute
  } else {
    delete event;
  }
}

} // namespace rdd
