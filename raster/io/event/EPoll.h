/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <iostream>
#include <utility>
#include <sys/epoll.h>

namespace rdd {

class EPoll {
 public:
  enum : uint32_t {
    kRead = EPOLLIN,
    kWrite = EPOLLOUT,
    kError = EPOLLERR | EPOLLHUP,
  };

  explicit EPoll(int size = kMaxEvents);

  ~EPoll();

  void add(int fd, uint32_t events);
  void modify(int fd, uint32_t events);
  void remove(int fd);

  int wait(int timeout);

  struct epoll_event get(int i) const {
    return events_[i];
  }

  static constexpr int kMaxEvents = 1024;

 private:
  void control(int op, int fd, uint32_t events);

  int fd_;
  int size_;
  epoll_event* events_;
};

} // namespace rdd
