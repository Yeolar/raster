/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/Fiber.h"
#include "raster/io/event/Event.h"
#include "raster/net/Processor.h"

namespace rdd {

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

} // namespace rdd
