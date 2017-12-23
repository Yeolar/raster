/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/GenericExecutor.h"
#include "raster/io/event/Event.h"
#include "raster/net/Processor.h"

namespace rdd {

class EventExecutor : public GenericExecutor {
public:
  EventExecutor(Event* event) : event_(event) {
    event_->setExecutor(this);
  }

  virtual ~EventExecutor() {}

  void handle() {
    event_->processor()->run();
    event_->setState(Event::kToWrite);
  }

  Event* event() const { return event_; }

private:
  Event* event_;
};

} // namespace rdd
