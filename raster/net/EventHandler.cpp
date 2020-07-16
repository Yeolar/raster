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

#include "raster/net/EventHandler.h"

#include <accelerator/Logging.h>
#include <accelerator/Monitor.h>

#include "raster/net/Event.h"

namespace raster {

void EventHandler::onConnect(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  assert(event->state() == acc::EventBase::kConnect);

  if (event->isConnectTimeout()) {
    event->setState(acc::EventBase::kTimeout);
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
    event->setState(acc::EventBase::kError);
    onError(event);
    return;
  }
  ACCLOG(V1) << *event << " connect: complete";
  event->setState(acc::EventBase::kToWrite);
}

void EventHandler::onListen(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  assert(event->state() == acc::EventBase::kListen);

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
  evnew->setState(acc::EventBase::kNext);
  ACCLOG(V2) << *evnew << " add event";
  loop_->pushEvent(evnew);
  loop_->dispatchEvent(evnew);
}

void EventHandler::onRead(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  event->setState(acc::EventBase::kReading);

  int r = event->readData();
  switch (r) {
    case 1: {
      ACCLOG(V1) << *event << " read: complete";
      event->setState(acc::EventBase::kReaded);
      onComplete(event);
      break;
    }
    case -1: {
      ACCLOG(ERROR) << *event << " read: close for error: "
        << strerror(errno);
      event->setState(acc::EventBase::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isReadTimeout()) {
        event->setState(acc::EventBase::kTimeout);
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
      close(event);
      break;
    }
    default: break;
  }
}

void EventHandler::onWrite(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  event->setState(acc::EventBase::kWriting);

  int r = event->writeData();
  switch (r) {
    case 1: {
      ACCLOG(V1) << *event << " write: complete";
      event->setState(acc::EventBase::kWrited);
      onComplete(event);
      break;
    }
    case -1: {
      ACCLOG(ERROR) << *event << " write: close for error: "
        << strerror(errno);
      event->setState(acc::EventBase::kError);
      onError(event);
      break;
    }
    case -2: {
      if (event->isWriteTimeout()) {
        event->setState(acc::EventBase::kTimeout);
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

void EventHandler::onTimeout(acc::EventBase *ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  assert(event->state() == acc::EventBase::kTimeout);

  ACCMON_CNT("conn.timeout-" + event->label());
  close(event);
}

void EventHandler::close(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  loop_->popEvent(event);

  if (event->socket()->isClient()) {
    event->setState(acc::EventBase::kFail);
    event->callbackOnClose();  // execute
  } else {
    delete event;
  }
}

void EventHandler::onComplete(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  // for server: kReaded -> kWrited
  // for client: kWrited -> kReaded

  if (event->state() == acc::EventBase::kReaded) {
    if (event->isReadTimeout()) {
      event->setState(acc::EventBase::kTimeout);
      ACCLOG(WARN) << *event << " remove read timeout request: >"
        << event->timeoutOption().rtimeout;
      onTimeout(event);
      return;
    }

    loop_->popEvent(event);

    // on result
    if (event->socket()->isClient()) {
      ACCMON_CNT("conn.success-" + event->label());
      ACCMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
    }
    if (!event->isForward()) {
      event->callbackOnComplete();  // execute
    }
    return;
  }

  if (event->state() == acc::EventBase::kWrited) {
    if (event->isWriteTimeout()) {
      event->setState(acc::EventBase::kTimeout);
      ACCLOG(WARN) << *event << " remove write timeout request: >"
        << event->timeoutOption().wtimeout;
      onTimeout(event);
      return;
    }

    loop_->updateEvent(event, acc::EPoll::kRead);

    // server: wait next; client: wait response
    if (event->socket()->isServer()) {
      ACCMON_CNT("conn.success-" + event->label());
      ACCMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
      event->reset();
      event->setState(acc::EventBase::kNext);
    } else {
      event->setState(acc::EventBase::kToRead);
    }
    loop_->dispatchEvent(event);
    return;
  }
}

void EventHandler::onError(acc::EventBase* ev) {
  Event* event = reinterpret_cast<Event*>(ev);

  assert(event->state() == acc::EventBase::kError);

  ACCMON_CNT("conn.error-" + event->label());
  close(event);
}

} // namespace raster
