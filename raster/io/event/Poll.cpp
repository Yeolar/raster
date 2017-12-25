/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/Poll.h"
#include "raster/io/event/EventHandler.h"
#include "raster/util/Logging.h"

namespace rdd {

Poll::Poll(int size) : size_(size) {
  fd_ = epoll_create(size_);
  if (fd_ == -1) {
    RDDPLOG(ERROR) << "epoll_create failed";
  }
  events_ = (epoll_event*) malloc(sizeof(epoll_event) * size_);
  memset(events_, 0, sizeof(epoll_event) * size_);
}

Poll::~Poll() {
  if (fd_ != -1) {
    close();
  }
  fd_ = -1;
  free(events_);
}

void Poll::close() {
  int r = ::close(fd_);
  if (r == -1 && errno != EINTR) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): close failed";
  }
}

int Poll::poll(int timeout) {
  int r = epoll_wait(fd_, events_, size_, timeout);
  if (r == -1 && errno != EINTR) {
    RDDPLOG(ERROR) << "epoll_wait failed";
  }
  return r;
}

void Poll::control(int op, int fd, Event events, void* ptr) {
  epoll_event ev;
  ev.events = events.data;
  ev.data.ptr = ptr;
  int r = epoll_ctl(fd_, op, fd, &ev);
  if (r == -1) {
    if (op == EPOLL_CTL_DEL && errno == ENOENT) {
      return;
    }
    RDDPLOG(ERROR) << "epoll_ctl " << operationName(op)
      << " ev(" << events << ") on fd(" << fd << ") failed";
  }
}

std::ostream& operator<<(std::ostream& os, const Poll::Event& event) {
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

} // namespace rdd
