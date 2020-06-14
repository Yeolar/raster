/*
 * Copyright 2020 Yeolar
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

#include "raster/event/EventPoll.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <accelerator/Exception.h>
#include <accelerator/Logging.h>

#ifdef __linux__

namespace raster {

EventPoll::EventPoll(int size) : Poll(size) {
  fd_ = epoll_create(1024);
  if (fd_ == -1) {
    acc::throwSystemError("epoll_create failed");
  }
  events_.resize(size);
}

EventPoll::~EventPoll() {
  close(fd_);
}

bool EventPoll::add(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = 0;
  if (mask & EventBase::kRead)
    ee.events |= EPOLLIN;
  if (mask & EventBase::kWrite)
    ee.events |= EPOLLOUT;
  if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ee) == -1) {
    ACCPLOG(ERROR) << "epoll_ctl(ADD," << fd << "," << mask << ") failed";
    return false;
  }
  eventMap_[fd]->setMask(mask);
  return true;
}

bool EventPoll::modify(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = 0;
  if (mask & EventBase::kRead)
    ee.events |= EPOLLIN;
  if (mask & EventBase::kWrite)
    ee.events |= EPOLLOUT;
  if (epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &ee) == -1) {
    ACCPLOG(ERROR) << "epoll_ctl(MOD," << fd << "," << mask << ") failed";
    return false;
  }
  eventMap_[fd]->setMask(mask);
  return true;
}

bool EventPoll::remove(int fd) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = 0;
  if (epoll_ctl(fd_, EPOLL_CTL_DEL, fd, &ee) == -1) {
    ACCPLOG(ERROR) << "epoll_ctl(DEL," << fd << ") failed";
    return false;
  }
  eventMap_[fd]->setMask(EventBase::kNone);
  return true;
}

int EventPoll::wait(int timeout) {
  int n = epoll_wait(fd_, events_.data(), size_, timeout);
  if (n == -1 && errno != EINTR) {
    ACCPLOG(ERROR) << "epoll_wait failed";
    n = 0;
  }
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      int mask = 0;
      struct epoll_event& ee = events_[i];
      if (ee.events & EPOLLIN ||
          ee.events & EPOLLERR ||
          ee.events & EPOLLHUP)
        mask |= EventBase::kRead;
      if (ee.events & EPOLLOUT ||
          ee.events & EPOLLERR ||
          ee.events & EPOLLHUP)
        mask |= EventBase::kWrite;
      fired_[i].fd = ee.data.fd;
      fired_[i].mask = mask;
    }
  }
  return n;
}

}  // namespace raster

#endif
