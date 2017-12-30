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
  EventTask(std::unique_ptr<Event> event)
    : event_(std::move(event)) {
    event_->setTask(this);
  }

  ~EventTask() override {}

  void handle() override {
    event_->processor()->run();
  }

  void onExit() override {
    event_->setState(Event::kToWrite);
    Singleton<Actor>::get()->addEvent(std::move(event_));
  }

  Event* event() const { return event_.get(); }

 private:
  std::unique_ptr<Event> event_;
};

} // namespace rdd
