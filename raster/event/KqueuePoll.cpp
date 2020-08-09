/*
 * Copyright 2020 Yeolar
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

#include "raster/event/KqueuePoll.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <accelerator/Exception.h>
#include <accelerator/Logging.h>
#include <accelerator/Time.h>

#ifdef __APPLE__

namespace raster {

KqueuePoll::KqueuePoll(int size) : Poll(size) {
  fd_ = kqueue();
  if (fd_ == -1) {
    acc::throwSystemError("kqueue failed");
  }
  events_.resize(size);
}

KqueuePoll::~KqueuePoll() {
  close(fd_);
}

namespace {

void addRead(struct kevent* ke, int fd, int kfd) {
  EV_SET(ke, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
  if (kevent(kfd, ke, 1, nullptr, 0, nullptr) == -1) {
    ACCPLOG(ERROR) << "kevent(ADD," << fd << ",READ) failed";
  }
}

void delRead(struct kevent* ke, int fd, int kfd) {
  EV_SET(ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  if (kevent(kfd, ke, 1, nullptr, 0, nullptr) == -1) {
    ACCPLOG(ERROR) << "kevent(DEL," << fd << ",READ) failed";
  }
}

void addWrite(struct kevent* ke, int fd, int kfd) {
  EV_SET(ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
  if (kevent(kfd, ke, 1, nullptr, 0, nullptr) == -1) {
    ACCPLOG(ERROR) << "kevent(ADD," << fd << ",WRITE) failed";
  }
}

void delWrite(struct kevent* ke, int fd, int kfd) {
  EV_SET(ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  if (kevent(kfd, ke, 1, nullptr, 0, nullptr) == -1) {
    ACCPLOG(ERROR) << "kevent(DEL," << fd << ",WRITE) failed";
  }
}

}  // namespace

bool KqueuePoll::add(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct kevent ke;
  if (mask & EventBase::kRead) {
    addRead(&ke, fd, fd_);
  }
  if (mask & EventBase::kWrite) {
    addWrite(&ke, fd, fd_);
  }
  eventMap_[fd]->setMask(mask);
  return true;
}

bool KqueuePoll::modify(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct kevent ke;
  if ((eventMap_[fd]->mask() & EventBase::kRead)
      != (mask & EventBase::kRead)) {
    if (mask & EventBase::kRead) {
      addRead(&ke, fd, fd_);
    } else {
      delRead(&ke, fd, fd_);
    }
  }
  if ((eventMap_[fd]->mask() & EventBase::kWrite)
      != (mask & EventBase::kWrite)) {
    if (mask & EventBase::kWrite) {
      addWrite(&ke, fd, fd_);
    } else {
      delWrite(&ke, fd, fd_);
    }
  }
  eventMap_[fd]->setMask(mask);
  return true;
}

bool KqueuePoll::remove(int fd) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  struct kevent ke;
  if (eventMap_[fd]->mask() & EventBase::kRead) {
    delRead(&ke, fd, fd_);
  }
  if (eventMap_[fd]->mask() & EventBase::kWrite) {
    delWrite(&ke, fd, fd_);
  }
  eventMap_[fd]->setMask(EventBase::kNone);
  return true;
}

int KqueuePoll::wait(int timeout) {
  int n;
  if (timeout != -1) {
    struct timespec ts = acc::toTimespec(timeout * 1000000);
    n = kevent(fd_, nullptr, 0, events_.data(), size_, &ts);
  } else {
    n = kevent(fd_, nullptr, 0, events_.data(), size_, nullptr);
  }
  if (n == -1 && errno != EINTR) {
    ACCPLOG(ERROR) << "kevent failed";
    n = 0;
  }
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      int mask = 0;
      struct kevent& ke = events_[i];
      if (ke.filter == EVFILT_READ)
        mask |= EventBase::kRead;
      if (ke.filter == EVFILT_WRITE)
        mask |= EventBase::kWrite;
      fired_[i].fd = ke.ident;
      fired_[i].mask = mask;
    }
  }
  return n;
}

}  // namespace raster

#endif
