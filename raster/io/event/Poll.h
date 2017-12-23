/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <iostream>
#include <utility>
#include <sys/epoll.h>

namespace rdd {

class Poll {
public:
  struct Event {
    uint32_t data;
    Event(uint32_t events) : data(events) {}
  };

  static constexpr int kMaxEvents = 1024;

  explicit Poll(int size);

  ~Poll();

  void close();

  void add(int fd, Event events, void* ptr) {
    control(EPOLL_CTL_ADD, fd, events, ptr);
  }

  void remove(int fd) {
    control(EPOLL_CTL_DEL, fd, 0, nullptr);
  }

  int poll(int timeout);

  struct epoll_event get(int i) const {
    return events_[i];
  }

private:
  const char* operationName(int op) const {
    switch (op) {
      case EPOLL_CTL_ADD: return "ADD";
      case EPOLL_CTL_DEL: return "DEL";
    }
    return "unknown";
  }

  void control(int op, int fd, Event events, void* ptr);

  int fd_;
  int size_;
  epoll_event* events_;
};

std::ostream& operator<<(std::ostream& os, const Poll::Event& event);

} // namespace rdd
