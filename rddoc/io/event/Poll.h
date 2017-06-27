/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <sys/epoll.h>

namespace rdd {

class Poll {
public:
  static constexpr int MAX_EVENTS = 1024;

  explicit Poll(int size);

  ~Poll();

  void close();

  void add(int fd, uint32_t events, void* ptr) {
    control(EPOLL_CTL_ADD, fd, events, ptr);
  }

  void remove(int fd) {
    control(EPOLL_CTL_DEL, fd, 0, nullptr);
  }

  int poll(int timeout);

  uint32_t getData(int i, epoll_data_t& data) const {
    data = events_[i].data;
    return events_[i].events;
  }

private:
  const char* operationName(int op) const {
    switch (op) {
      case EPOLL_CTL_ADD: return "ADD";
      case EPOLL_CTL_DEL: return "DEL";
    }
    return "unknown";
  }

  void control(int op, int fd, uint32_t events, void* ptr);

  int fd_;
  int size_;
  epoll_event* events_;
};

}

