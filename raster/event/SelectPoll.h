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

#pragma once

#include <sys/select.h>

#include "raster/event/Poll.h"

namespace raster {

class SelectPoll : public Poll {
 public:
  SelectPoll(int size);
  ~SelectPoll() override;

  bool add(int fd, int mask) override;
  bool modify(int fd, int mask) override;
  bool remove(int fd) override;

  int wait(int timeout) override;

 private:
  int fd_;
  fd_set rfds_, wfds_;
};

}  // namespace raster
