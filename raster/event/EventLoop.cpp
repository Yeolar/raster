/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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

namespace raster {

#define ACC_EVENT_MONKEY_GEN(x) \
  x(AVG, LoopEvent),            \
  x(MAX, LoopEventMax),         \
  x(AVG, LoopCost),             \
  x(MAX, LoopCostMax),          \
  x(NON, Max)

ACCMON_KEY(EventMonitorKey, ACC_EVENT_MONKEY_GEN);

class WakerEvent : public EventBase {
 public:
  WakerEvent(int fd) : EventBase(TimeoutOption()), fd_(fd) {}

  int fd() const override {
    return fd_;
  }

  std::string str() const override {
    return acc::to<std::string>("Waker(", fd_, ")");
  }

 private:
  int fd_;
};

EventLoop::EventLoop(int pollSize, int pollTimeout)
    : timeout_(pollTimeout),
      stop_(false),
      loopThread_() {
  int fd = waker_.fd();
  poll_ = Poll::create(pollSize);
  poll_->event(fd) = new WakerEvent(fd);
  poll_->add(fd, EventBase::kRead);
}

EventLoop::~EventLoop() {
  delete poll_->event(waker_.fd());
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

    events_.sweep([&](EventBase* ev) {
      ACCLOG(V2) << *ev << " add event";
      pushEvent(ev);
      dispatchEvent(ev);
    });

    callbacks_.sweep([&](VoidFunc cb) {
      cb();
    });

    checkTimeoutEvents();

    int n = poll_->wait(timeout_);
    if (n > 0) {
      const Poll::FdMask* p = poll_->firedFds();
      for (int i = 0; i < n; ++i) {
        int fd = p[i].fd;
        if (fd == waker_.fd()) {
          waker_.consume();
        } else {
          EventBase* event = poll_->event(fd);
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
  events_.insertHead(event);
  waker_.wake();
}

void EventLoop::addCallback(VoidFunc&& callback) {
  callbacks_.insertHead(std::move(callback));
  waker_.wake();
}

void EventLoop::pushEvent(EventBase* event) {
  poll_->event(event->fd()) = event;
}

void EventLoop::popEvent(EventBase* event) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  ACCLOG(V2) << *event << " remove event";
  poll_->remove(event->fd());
  poll_->event(event->fd()) = nullptr;
}

void EventLoop::updateEvent(EventBase *event, int mask) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  ACCLOG(V2) << *event << " update event";
  poll_->modify(event->fd(), mask);
}

void EventLoop::dispatchEvent(EventBase* event) {
  switch (event->state()) {
    case EventBase::kListen: {
      listenFds_.push_back(event->fd());
      poll_->add(event->fd(), EventBase::kRead);
      break;
    }
    case EventBase::kNext: {
      ACCLOG(V2) << *event << " add edeadline";
      deadlineHeap_.push(event->edeadline());
      // already update epoll
      break;
    }
    case EventBase::kToRead: {
      ACCLOG(V2) << *event << " add rdeadline";
      deadlineHeap_.push(event->rdeadline());
      // already update epoll
      break;
    }
    case EventBase::kConnect: {
      ACCLOG(V2) << *event << " add cdeadline";
      deadlineHeap_.push(event->cdeadline());
      poll_->add(event->fd(), EventBase::kWrite);
      break;
    }
    case EventBase::kToWrite: {
      ACCLOG(V2) << *event << " add wdeadline";
      deadlineHeap_.push(event->wdeadline());
      poll_->add(event->fd(), EventBase::kWrite);
      break;
    }
    default:
      ACCLOG(ERROR) << *event << " cannot add event";
      return;
  }
}

void EventLoop::restartEvent(EventBase* event) {
  ACCLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);

  event->restart();  // the next request

  ACCLOG(V2) << *event << " add rdeadline";
  deadlineHeap_.push(event->rdeadline());
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
        << timeout.deadline - event->startTime();
      handler_->onTimeout(event);
    }
  }
}

} // namespace raster
