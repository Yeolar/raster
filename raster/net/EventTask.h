/*
 * Copyright 2017 Yeolar
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

#include "raster/coroutine/Fiber.h"
#include "raster/net/Event.h"
#include "raster/net/Processor.h"

namespace raster {

class EventTask : public Fiber::Task {
 public:
  EventTask(Event* event) : event_(event) {
    event_->setTask(this);
  }

  ~EventTask() override {}

  void handle() override {
    event_->processor()->run();
    event_->setState(Event::kToWrite);
  }

  Event* event() const { return event_; }

 private:
  Event* event_;
};

} // namespace raster
