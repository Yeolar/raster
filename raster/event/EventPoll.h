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

#pragma once

#include "raster/event/Poll.h"

#ifdef __linux__

#include <sys/epoll.h>

namespace raster {

class EventPoll : public Poll {
 public:
  EventPoll(int size);
  ~EventPoll() override;

  bool add(int fd, int mask) override;
  bool modify(int fd, int mask) override;
  bool remove(int fd) override;

  int wait(int timeout) override;

 private:
  int fd_;
  std::vector<struct epoll_event> events_;
};

}  // namespace raster

#endif
