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
