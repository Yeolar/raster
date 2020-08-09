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

#include "raster/event/EventBase.h"

namespace raster {

class Event : public EventBase {
 public:
  Event(int fd, const TimeoutOption& timeoutOpt = TimeoutOption())
      : EventBase(timeoutOpt), fd_(fd) {
  }

  int fd() const override {
    return fd_;
  }

  std::string str() const override {
    return acc::to<std::string>("Event(", fd_, ")");
  }

 private:
  int fd_;
};

} // namespace raster