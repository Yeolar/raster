/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/Actor.h"
#include "rddoc/net/EventLoop.h"
#include "rddoc/net/Protocol.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

void EventLoop::listen(const std::shared_ptr<Channel>& channel, int backlog) {
  int port = channel->peer().port;
  auto socket = std::make_shared<Socket>(0);
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking()) ||
      !(socket->bind(port)) ||
      !(socket->listen(backlog))) {
    throw std::runtime_error("socket listen failed");
  }
  Event *event = new Event(channel, socket);
  if (!event) {
    throw std::runtime_error("create listening event failed");
  }
  listen_fds_.push_back(socket->fd());
  event->setType(Event::LISTEN);
  dispatchEvent(event);
  RDD_EVLOG(INFO, event) << "listen on port=" << port;
}

void EventLoop::start() {
  running_ = true;
  while (running_) {
    uint64_t t0 = timestampNow();

    std::vector<Event*> events;
    {
      LockGuard guard(event_lock_);
      events.swap(events_);
    }
    for (auto& e : events) {
      dispatchEvent(e);
    }
    std::vector<std::pair<PtrCallback, void*>> callbacks;
    {
      LockGuard guard(callback_lock_);
      callbacks.swap(callbacks_);
    }
    for (auto& kv : callbacks) {
      actor_->addCallbackTask(kv.first, kv.second);
    }

    checkTimeoutEvent();

    int n = poll_.poll(timeout_);
    if (n >= 0) {
      for (int i = 0; i < n; ++i) {
        handleEvent(i);
      }
    }
    uint64_t cost = timePassed(t0) / 1000;

    RDDMON_AVG("loopevent", n);
    RDDMON_MAX("loopevent.max", n);
    RDDMON_AVG("loopcost", cost);
    RDDMON_MAX("loopcost.max", cost);
    RDDMON_AVG("totaltask", Task::count());
    RDDMON_AVG("connection", Socket::count());

    actor_->monitoring();
  }
}

void EventLoop::stop() {
  poll_.remove(waker_.fd());
  for (auto& fd : listen_fds_) {
    poll_.remove(fd);
  }
  running_ = false;
}

void EventLoop::addEvent(Event* event) {
  {
    LockGuard guard(event_lock_);
    events_.push_back(event);
  }
  waker_.wake();
}

void EventLoop::addCallback(const PtrCallback& callback, void* ptr) {
  {
    LockGuard guard(callback_lock_);
    callbacks_.push_back(std::make_pair(callback, ptr));
  }
  waker_.wake();
}

void EventLoop::dispatchEvent(Event *event) {
  RDD_EVLOG(V2, event)
    << "add event on task(" << (void*)event->task() << ")";
  switch (event->type()) {
    case Event::LISTEN:
      addListenEvent(event); break;
    case Event::NEXT:
    case Event::TOREAD:
      addReadEvent(event); break;
    case Event::CONNECT:
    case Event::TOWRITE:
      addWriteEvent(event); break;
    default:
      RDD_EVLOG(ERROR, event) << "cannot add event";
      break;
  }
}

void EventLoop::handleEvent(int i) {
  epoll_data_t edata;
  uint32_t etype = poll_.getData(i, edata);
  Event* event = (Event*) edata.ptr;
  if (!event) {
    return;
  }
  RDD_EVLOG(V2, event) << "handle event";
  switch (event->type()) {
    case Event::LISTEN:
      handleListen(event); break;
    case Event::CONNECT:
      handleConnect(event); break;
    case Event::NEXT:
    case Event::TOREAD:
    case Event::READING:
      handleRead(event); break;
    case Event::TOWRITE:
    case Event::WRITING:
      handleWrite(event); break;
    case Event::TIMEOUT:
      handleTimeout(event); break;
    case Event::WAKER:
      waker_.consume(); break;
    default:
      RDD_EVLOG(ERROR, event) << "error event, type=" << etype;
      closePeer(event);
      break;
  }
}

void EventLoop::addListenEvent(Event *event) {
  assert(event->type() == Event::LISTEN);
  poll_.add(event->fd(), EPOLLIN, event);
}

void EventLoop::addReadEvent(Event *event) {
  assert(event->type() == Event::NEXT ||
         event->type() == Event::TOREAD);
  RDD_EVLOG(V2, event) << "add e/rdeadline";
  deadline_heap_.push(event->type() == Event::NEXT ?
                      event->edeadline() : event->rdeadline());
  poll_.add(event->fd(), EPOLLIN, event);
}

void EventLoop::addWriteEvent(Event *event) {
  assert(event->type() == Event::CONNECT ||
         event->type() == Event::TOWRITE);
  RDD_EVLOG(V2, event) << "add wdeadline";
  // TODO epoll_wait interval
  //deadline_heap_.push(event->type() == Event::CONNECT ?
  //                    event->cdeadline() : event->wdeadline());
  deadline_heap_.push(event->wdeadline());
  poll_.add(event->fd(), EPOLLOUT, event);
}

void EventLoop::removeEvent(Event *event) {
  RDD_EVLOG(V2, event) << "remove deadline";
  deadline_heap_.erase(event);
  RDD_EVLOG(V2, event) << "remove event";
  poll_.remove(event->fd());
}

void EventLoop::handleListen(Event* event) {
  assert(event->type() == Event::LISTEN);
  auto socket = event->socket()->accept();
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking())) {
    return;
  }
  if (actor_->exceedConnectionLimit()) {
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
    handleTimeout(evnew);
    return;
  }
  evnew->setType(Event::NEXT);
  dispatchEvent(evnew);
}

void EventLoop::handleConnect(Event* event) {
  assert(event->type() == Event::CONNECT);
  if (event->isConnectTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove connect timeout request: >"
      << event->timeoutOption().ctimeout;
    handleTimeout(event);
    return;
  }
  int err = 1;
  event->socket()->getError(err);
  if (err != 0) {
    RDD_EVLOG(ERROR, event) << "connect: close for error: " << strerror(errno);
    event->setType(Event::ERROR);
    handleError(event);
    return;
  }
  RDD_EVLOG(V1, event) << "connect: complete";
  event->setType(Event::TOWRITE);
}

void EventLoop::handleTimeout(Event *event) {
  assert(event->type() == Event::TIMEOUT);
  RDDMON_CNT("conn.timeout-" + event->label());
  closePeer(event);
}

void EventLoop::handleError(Event* event) {
  assert(event->type() == Event::ERROR);
  RDDMON_CNT("conn.error-" + event->label());
  closePeer(event);
}

void EventLoop::closePeer(Event* event) {
  removeEvent(event);
  if (event->role() == Socket::CLIENT) {
    event->setType(Event::FAIL);
    actor_->addEventTask(event);
  } else {
    delete event;
  }
}

void EventLoop::handleComplete(Event* event) {
  if (event->type() == Event::READED && event->isReadTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove read timeout request: >"
      << event->timeoutOption().rtimeout;
    handleTimeout(event);
    return;
  }
  if (event->type() == Event::WRITED && event->isWriteTimeout()) {
    event->setType(Event::TIMEOUT);
    RDD_EVTLOG(WARN, event) << "remove write timeout request: >"
      << event->timeoutOption().wtimeout;
    handleTimeout(event);
    return;
  }
  removeEvent(event);

  // for server: READED -> WRITED
  // for client: WRITED -> READED

  switch (event->type()) {
    // handle result
    case Event::READED:
    {
      if (event->role() == Socket::CLIENT) {
        RDDMON_CNT("conn.success-" + event->label());
        RDDMON_AVG("conn.cost-" + event->label(), event->cost() / 1000);
        if (event->isForward()) {
          delete event;
        }
      }
      actor_->addEventTask(event);
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
      dispatchEvent(event);
      break;
    }
    default: break;
  }
}

void EventLoop::handleRead(Event* event) {
  if (event->type() == Event::NEXT) {
    RDD_EVLOG(V2, event) << "remove deadline";
    deadline_heap_.erase(event);
    event->restart();  // the next request
    RDD_EVLOG(V2, event) << "add rdeadline";
    deadline_heap_.push(event->rdeadline());
  }
  event->setType(Event::READING);
  int r = event->readData();
  switch (r) {
    case -1:
    {
      if (event->type() == Event::TIMEOUT) {
        RDD_EVTLOG(WARN, event) << "remove read timeout request: >"
          << event->timeoutOption().rtimeout;
        handleTimeout(event);
      } else {
        RDD_EVLOG(ERROR, event) << "read: close for error: "
          << strerror(errno);
        event->setType(Event::ERROR);
        handleError(event);
      }
      break;
    }
    case 0:
    {
      RDD_EVLOG(V1, event) << "read: complete";
      event->setType(Event::READED);
      handleComplete(event);
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

void EventLoop::handleWrite(Event* event) {
  event->setType(Event::WRITING);
  int r = event->writeData();
  switch (r) {
    case -1:
    {
      if (event->type() == Event::TIMEOUT) {
        RDD_EVTLOG(WARN, event) << "remove write timeout request: >"
          << event->timeoutOption().wtimeout;
        handleTimeout(event);
      } else {
        RDD_EVLOG(ERROR, event) << "write: close for error: "
          << strerror(errno);
        event->setType(Event::ERROR);
        handleError(event);
      }
      break;
    }
    case 0:
    {
      RDD_EVLOG(V1, event) << "write: complete";
      event->setType(Event::WRITED);
      handleComplete(event);
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

void EventLoop::checkTimeoutEvent() {
  uint64_t now = timestampNow();
  while (true) {
    auto timeout = deadline_heap_.pop(now);
    Event* event = timeout.data;
    if (!event) {
      break;
    }
    if (timeout.repeat && Socket::count() < Socket::LCOUNT) {
      timeout.deadline += Socket::LTIMEOUT;
      deadline_heap_.push(timeout);
    }
    else {
      RDD_EVLOG(V2, event) << "pop deadline";
      event->setType(Event::TIMEOUT);
      RDD_EVTLOG(WARN, event) << "remove timeout event: >"
        << timeout.deadline - event->starttime();
      handleTimeout(event);
    }
  }
}

}

