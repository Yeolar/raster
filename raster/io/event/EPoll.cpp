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

#include "raster/io/event/EPoll.h"

#include "raster/util/Exception.h"
#include "raster/util/Logging.h"

namespace rdd {

EPoll::EPoll(int size)
  : fd_(::epoll_create(size)),
    size_(size) {
  if (fd_ == -1) {
    throwSystemError("epoll_create(", size, ") failed");
  }
  size_t n = sizeof(epoll_event) * size;
  events_ = (epoll_event*) malloc(n);
  ::memset(events_, 0, n);
}

EPoll::~EPoll() {
  ::close(fd_);
  free(events_);
}

void EPoll::add(int fd, uint32_t events) {
  control(EPOLL_CTL_ADD, fd, events);
}

void EPoll::modify(int fd, uint32_t events) {
  control(EPOLL_CTL_MOD, fd, events);
}

void EPoll::remove(int fd) {
  control(EPOLL_CTL_DEL, fd, 0);
}

int EPoll::wait(int timeout) {
  int r = ::epoll_wait(fd_, events_, size_, timeout);
  if (r == -1 && errno != EINTR) {
    RDDPLOG(ERROR) << "epoll_wait failed";
  }
  return r;
}

namespace {

const char* operationName(int op) {
  switch (op) {
    case EPOLL_CTL_ADD: return "ADD";
    case EPOLL_CTL_DEL: return "DEL";
  }
  return "unknown";
}

struct PrettyEvent {
  uint32_t data;
};

std::ostream& operator<<(std::ostream& os, const PrettyEvent& event) {
#define OUTPUTEVENTNAME_IMPL(e) \
  if (event.data & e) os << sepchar() << #e

  int more = 0;
  auto sepchar = [&]() -> char {
    return (more || more++) ? '|' : '(';
  };

  OUTPUTEVENTNAME_IMPL(EPOLLIN);
  OUTPUTEVENTNAME_IMPL(EPOLLPRI);
  OUTPUTEVENTNAME_IMPL(EPOLLOUT);
  OUTPUTEVENTNAME_IMPL(EPOLLRDNORM);
  OUTPUTEVENTNAME_IMPL(EPOLLRDBAND);
  OUTPUTEVENTNAME_IMPL(EPOLLWRNORM);
  OUTPUTEVENTNAME_IMPL(EPOLLWRBAND);
  OUTPUTEVENTNAME_IMPL(EPOLLMSG);
  OUTPUTEVENTNAME_IMPL(EPOLLERR);
  OUTPUTEVENTNAME_IMPL(EPOLLHUP);
  OUTPUTEVENTNAME_IMPL(EPOLLRDHUP);
  OUTPUTEVENTNAME_IMPL(EPOLLWAKEUP);
  OUTPUTEVENTNAME_IMPL(EPOLLONESHOT);
  OUTPUTEVENTNAME_IMPL(EPOLLET);

  os << ")";
  return os;

#undef OUTPUTEVENTNAME_IMPL
}

} // namespace

void EPoll::control(int op, int fd, uint32_t events) {
  epoll_event ee;
  ee.data.fd = fd;
  ee.events = events;
  int r = epoll_ctl(fd_, op, fd, &ee);
  if (r == -1) {
    if (op == EPOLL_CTL_DEL && errno == ENOENT) {
      return;
    }
    RDDPLOG(ERROR) << "epoll_ctl(" << operationName(op)
                   << ", " << fd << ", " << PrettyEvent{events} << ") failed";
  }
}

} // namespace rdd
