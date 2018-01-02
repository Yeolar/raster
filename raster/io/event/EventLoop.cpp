/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/EventLoop.h"

#include "raster/framework/Monitor.h"
#include "raster/net/Channel.h"
#include "raster/util/ScopeGuard.h"

namespace rdd {

EventLoop::EventLoop(int pollSize, int pollTimeout)
  : poll_(pollSize),
    timeout_(pollTimeout),
    stop_(false),
    loopThread_(),
    handler_(this) {
  poll_.add(waker_.fd(), EPoll::kRead);
}

void EventLoop::loop() {
  return loopBody();
}

void EventLoop::loopOnce() {
  return loopBody(true);
}

void EventLoop::loopBody(bool once) {
  RDDLOG(V5) << "EventLoop(): Starting loop.";

  loopThread_.store(std::this_thread::get_id(), std::memory_order_release);

  while (!stop_.load(std::memory_order_acquire)) {
    uint64_t t0 = timestampNow();

    std::vector<Event*> events;
    {
      std::lock_guard<std::mutex> guard(eventsLock_);
      events.swap(events_);
    }
    for (auto& ev : events) {
      RDDLOG(V2) << *ev << " add event";
      pushEvent(ev);
      dispatchEvent(ev);
    }

    std::vector<VoidFunc> callbacks;
    {
      std::lock_guard<std::mutex> guard(callbacksLock_);
      callbacks.swap(callbacks_);
    }
    for (auto& cb : callbacks) {
      cb();
    }

    checkTimeoutEvents();

    int n = poll_.wait(timeout_);
    if (n > 0) {
      for (int i = 0; i < n; ++i) {
        struct epoll_event p = poll_.get(i);
        int fd = p.data.fd;
        if (fd == waker_.fd()) {
          waker_.consume();
        } else {
          Event* event = fdEvents_[fd];
          if (event) {
            RDDLOG(V2) << *event << " on event, type=" << p.events;
            switch (event->state()) {
              case Event::kListen:
                handler_.onListen(event); break;
              case Event::kConnect:
                handler_.onConnect(event); break;
              case Event::kNext:
                restartEvent(event);
              case Event::kToRead:
              case Event::kReading:
                handler_.onRead(event); break;
              case Event::kToWrite:
              case Event::kWriting:
                handler_.onWrite(event); break;
              case Event::kTimeout:
                handler_.onTimeout(event); break;
              default:
                RDDLOG(ERROR) << *event << " error event, type=" << p.events;
                handler_.closePeer(event);
                break;
            }
          }
        }
      }
      RDDMON_AVG("loopevent", n);
      RDDMON_MAX("loopevent.max", n);
    }

    uint64_t cost = timePassed(t0) / 1000;
    RDDMON_AVG("loopcost", cost);
    RDDMON_MAX("loopcost.max", cost);

    if (once) {
      break;
    }
  }

  stop_ = false;

  loopThread_.store({}, std::memory_order_release);
}

void EventLoop::stop() {
  poll_.remove(waker_.fd());
  for (auto& fd : listenFds_) {
    poll_.remove(fd);
  }
  stop_ = true;
}

void EventLoop::addEvent(Event* event) {
  {
    std::lock_guard<std::mutex> guard(eventsLock_);
    events_.push_back(event);
  }
  waker_.wake();
}

void EventLoop::addCallback(VoidFunc&& callback) {
  {
    std::lock_guard<std::mutex> guard(callbacksLock_);
    callbacks_.push_back(std::move(callback));
  }
  waker_.wake();
}

void EventLoop::dispatchEvent(Event* event) {
  switch (event->state()) {
    case Event::kListen: {
      listenFds_.push_back(event->fd());
      poll_.add(event->fd(), EPoll::kRead);
      break;
    }
    case Event::kNext:
    case Event::kToRead: {
      RDDLOG(V2) << *event << " add e/rdeadline";
      deadlineHeap_.push(event->state() == Event::kNext ?
                         event->edeadline() : event->rdeadline());
      // already update epoll
      break;
    }
    case Event::kConnect:
    case Event::kToWrite: {
      RDDLOG(V2) << *event << " add c/wdeadline";
      deadlineHeap_.push(event->state() == Event::kConnect ?
                         event->cdeadline() : event->wdeadline());
      poll_.add(event->fd(), EPoll::kWrite);
      break;
    }
    default:
      RDDLOG(ERROR) << *event << " cannot add event";
      return;
  }
}

void EventLoop::updateEvent(Event *event, uint32_t events) {
  RDDLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  RDDLOG(V2) << *event << " update event";
  poll_.modify(event->fd(), events);
}

void EventLoop::restartEvent(Event* event) {
  RDDLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  event->restart();  // the next request

  RDDLOG(V2) << *event << " add rdeadline";
  deadlineHeap_.push(event->rdeadline());
}

void EventLoop::pushEvent(Event* event) {
  fdEvents_[event->fd()] = event;
}

void EventLoop::popEvent(Event* event) {
  RDDLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  RDDLOG(V2) << *event << " remove event";
  poll_.remove(event->fd());
  fdEvents_[event->fd()] = nullptr;
}

void EventLoop::checkTimeoutEvents() {
  uint64_t now = timestampNow();

  while (true) {
    auto timeout = deadlineHeap_.pop(now);
    Event* event = timeout.data;
    if (!event) {
      break;
    }
    if (timeout.repeat && Socket::count() < FLAGS_net_conn_limit) {
      timeout.deadline += FLAGS_net_conn_timeout;
      deadlineHeap_.push(timeout);
    }
    else {
      RDDLOG(V2) << *event << " pop deadline";
      event->setState(Event::kTimeout);
      RDDLOG(WARN) << *event << " remove timeout event: >"
        << timeout.deadline - event->starttime();
      handler_.onTimeout(event);
    }
  }
}

} // namespace rdd
