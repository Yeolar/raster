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
    std::vector<VoidCallback> callbacks;
    {
      LockGuard guard(callback_lock_);
      callbacks.swap(callbacks_);
    }
    for (auto& callback : callbacks) {
      callback();
    }

    checkTimeoutEvent();

    int n = poll_.poll(timeout_);
    if (n >= 0) {
      for (int i = 0; i < n; ++i) {
        if (event->type() == Event::WAKER) {
          waker_.consume();
        } else {
          epoll_data_t edata;
          uint32_t etype = poll_.getData(i, edata);
          Event* event = (Event*) edata.ptr;
          if (event) {
            handle(event, etype);
          }
        }
      }
    }
    uint64_t cost = timePassed(t0) / 1000;

    RDDMON_AVG("loopevent", n);
    RDDMON_MAX("loopevent.max", n);
    RDDMON_AVG("loopcost", cost);
    RDDMON_MAX("loopcost.max", cost);

    //actor_->monitoring();
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

void EventLoop::addCallback(const VoidCallback& callback) {
  {
    LockGuard guard(callback_lock_);
    callbacks_.push_back(callback);
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

void EventLoop::restartEvent(Event* event) {
  RDD_EVLOG(V2, event) << "remove deadline";
  deadline_heap_.erase(event);
  event->restart();  // the next request
  RDD_EVLOG(V2, event) << "add rdeadline";
  deadline_heap_.push(event->rdeadline());
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
      onTimeout(event);
    }
  }
}

void EventLoop::closePeer(Event* event) {
  removeEvent(event);
  if (event->role() == Socket::CLIENT) {
    event->setType(Event::FAIL);
    Singleton<Actor>::get()->addEventTask(event);
  } else {
    delete event;
  }
}

}

