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
  int more = 0;
  auto sepchar = [&]() -> char {
    return (more || more++) ? '|' : '(';
  };
  if (event.data & EPOLLIN) os << sepchar() << "EPOLLIN";
  if (event.data & EPOLLPRI) os << sepchar() << "EPOLLPRI";
  if (event.data & EPOLLOUT) os << sepchar() << "EPOLLOUT";
  if (event.data & EPOLLRDNORM) os << sepchar() << "EPOLLRDNORM";
  if (event.data & EPOLLRDBAND) os << sepchar() << "EPOLLRDBAND";
  if (event.data & EPOLLWRNORM) os << sepchar() << "EPOLLWRNORM";
  if (event.data & EPOLLWRBAND) os << sepchar() << "EPOLLWRBAND";
  if (event.data & EPOLLMSG) os << sepchar() << "EPOLLMSG";
  if (event.data & EPOLLERR) os << sepchar() << "EPOLLERR";
  if (event.data & EPOLLHUP) os << sepchar() << "EPOLLHUP";
  if (event.data & EPOLLRDHUP) os << sepchar() << "EPOLLRDHUP";
  if (event.data & EPOLLWAKEUP) os << sepchar() << "EPOLLWAKEUP";
  if (event.data & EPOLLONESHOT) os << sepchar() << "EPOLLONESHOT";
  if (event.data & EPOLLET) os << sepchar() << "EPOLLET";
  os << ")";
  return os;
}

} // namespace rdd
