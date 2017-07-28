/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/coroutine/GenericExecutor.h"
#include "rddoc/io/event/Event.h"
#include "rddoc/net/Processor.h"

namespace rdd {

class EventExecutor : public GenericExecutor {
public:
  EventExecutor(Event* event) : event_(event) {
    event_->setExecutor(this);
  }

  virtual ~EventExecutor() {}

  void handle() {
    auto processor = event_->processor(true);
    processor->decodeData(event_);
    processor->run();
    processor->encodeData(event_);
    event_->setType(Event::TOWRITE);
  }

  Event* event() const { return event_; }

private:
  Event* event_;
};

} // namespace rdd
