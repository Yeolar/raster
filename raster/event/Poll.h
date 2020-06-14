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

#include <memory>
#include <vector>

#include "raster/Portability.h"
#include "raster/event/EventBase.h"

DECLARE_int32(peer_max_count);

namespace raster {

class Poll {
 public:
  struct FdMask {
    int fd;
    int mask;
  };

  static std::unique_ptr<Poll> create(int size = kMaxEvents);

  Poll(int size);
  virtual ~Poll() {}

  virtual bool add(int fd, int mask) = 0;
  virtual bool modify(int fd, int mask) = 0;
  virtual bool remove(int fd) = 0;

  virtual int wait(int timeout) = 0;

  const FdMask* firedFds() const;
  EventBase*& event(int i);

  static constexpr int kMaxEvents = 1024;

 protected:
  int size_;
  std::vector<FdMask> fired_;
  std::vector<EventBase*> eventMap_;
};

}  // namespace raster
