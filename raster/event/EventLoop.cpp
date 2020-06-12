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

#include "raster/event/EventLoop.h"

#include <accelerator/Logging.h>
#include <accelerator/Monitor.h>
#include <accelerator/ScopeGuard.h>

namespace raster {

#define ACC_EVENT_MONKEY_GEN(x) \
  x(AVG, LoopEvent),            \
  x(MAX, LoopEventMax),         \
  x(AVG, LoopCost),             \
  x(MAX, LoopCostMax),          \
  x(NON, Max)

ACCMON_KEY(EventMonitorKey, ACC_EVENT_MONKEY_GEN);

EventLoop::EventLoop(int pollSize, int pollTimeout)
  : timeout_(pollTimeout),
    stop_(false),
    loopThread_() {
  poll_ = Poll::create(pollSize);
  poll_->add(waker_.fd(), Poll::kRead);
}

void EventLoop::registerHandler(std::unique_ptr<EventHandlerBase> handler) {
  handler_ = std::move(handler);
}

void EventLoop::loop() {
  return loopBody();
}

void EventLoop::loopOnce() {
  return loopBody(true);
}

void EventLoop::loopBody(bool once) {
  ACCLOG(V5) << "EventLoop(): Starting loop.";

  loopThread_.store(std::this_thread::get_id(), std::memory_order_release);

  while (!stop_.load(std::memory_order_acquire)) {
    uint64_t t0 = acc::timestampNow();

    std::vector<EventBase*> events;
    {
      std::lock_guard<std::mutex> guard(eventsLock_);
      events.swap(events_);
    }
    for (auto& ev : events) {
      ACCLOG(V2) << *ev << " add event";
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

    int n = poll_->wait(timeout_);
    if (n > 0) {
      const Poll::Event* p = poll_->firedEvents();
      for (int i = 0; i < n; ++i) {
        int fd = p[i].fd;
        if (fd == waker_.fd()) {
          waker_.consume();
        } else {
          EventBase* event = fdEvents_[fd];
          if (event) {
            ACCLOG(V2) << *event << " on event, type=" << p[i].mask;
            switch (event->state()) {
              case EventBase::kConnect:
                handler_->onConnect(event); break;
              case EventBase::kListen:
                handler_->onListen(event); break;
              case EventBase::kNext:
                restartEvent(event);
              case EventBase::kToRead:
              case EventBase::kReading:
                handler_->onRead(event); break;
              case EventBase::kToWrite:
              case EventBase::kWriting:
                handler_->onWrite(event); break;
              case EventBase::kTimeout:
                handler_->onTimeout(event); break;
              default:
                ACCLOG(ERROR) << *event << " error event, type=" << p[i].mask;
                handler_->close(event);
                break;
            }
          }
        }
      }
      ACCMON_ADD(EventMonitorKey, kLoopEvent, n);
      ACCMON_ADD(EventMonitorKey, kLoopEventMax, n);
    }

    uint64_t cost = acc::elapsed(t0) / 1000;
    ACCMON_ADD(EventMonitorKey, kLoopCost, cost);
    ACCMON_ADD(EventMonitorKey, kLoopCostMax, cost);

    if (once) {
      break;
    }
  }

  stop_ = false;

  loopThread_.store({}, std::memory_order_release);
}

void EventLoop::stop() {
  poll_->remove(waker_.fd());
  for (auto& fd : listenFds_) {
    poll_->remove(fd);
  }
  stop_ = true;
}

void EventLoop::addEvent(EventBase* event) {
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

void EventLoop::dispatchEvent(EventBase* event) {
  switch (event->state()) {
    case EventBase::kListen: {
      listenFds_.push_back(event->fd());
      poll_->add(event->fd(), Poll::kRead);
      break;
    }
    case EventBase::kNext:
    case EventBase::kToRead: {
      ACCLOG(V2) << *event << " add e/rdeadline";
      deadlineHeap_.push(event->state() == EventBase::kNext ?
                         event->edeadline() : event->rdeadline());
      // already update epoll
      break;
    }
    case EventBase::kConnect:
    case EventBase::kToWrite: {
      ACCLOG(V2) << *event << " add c/wdeadline";
      deadlineHeap_.push(event->state() == EventBase::kConnect ?
                         event->cdeadline() : event->wdeadline());
      poll_->add(event->fd(), Poll::kWrite);
      break;
    }
    default:
      ACCLOG(ERROR) << *event << " cannot add event";
      return;
  }
}

void EventLoop::updateEvent(EventBase *event, uint32_t events) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  ACCLOG(V2) << *event << " update event";
  poll_->modify(event->fd(), events);
}

void EventLoop::restartEvent(EventBase* event) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  event->restart();  // the next request

  ACCLOG(V2) << *event << " add rdeadline";
  deadlineHeap_.push(event->rdeadline());
}

void EventLoop::pushEvent(EventBase* event) {
  fdEvents_[event->fd()] = event;
}

void EventLoop::popEvent(EventBase* event) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  ACCLOG(V2) << *event << " remove event";
  poll_->remove(event->fd());
  fdEvents_[event->fd()] = nullptr;
}

void EventLoop::checkTimeoutEvents() {
  uint64_t now = acc::timestampNow();

  while (true) {
    auto timeout = deadlineHeap_.pop(now);
    EventBase* event = timeout.data;
    if (!event) {
      break;
    }
    if (timeout.repeat) {
      timeout.deadline += FLAGS_event_lp_timeout;
      deadlineHeap_.push(timeout);
    }
    else {
      ACCLOG(V2) << *event << " pop deadline";
      event->setState(EventBase::kTimeout);
      ACCLOG(WARN) << *event << " remove timeout event: >"
        << timeout.deadline - event->starttime();
      handler_->onTimeout(event);
    }
  }
}

} // namespace raster
