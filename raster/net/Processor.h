/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>

#include "raster/io/event/Event.h"

namespace rdd {

class Event;

class Processor {
public:
  Processor(Event* event) : event_(event) {}
  virtual ~Processor() {}

  virtual bool decodeData() = 0;
  virtual bool encodeData() = 0;
  virtual bool run() = 0;

  template <class T = Event>
  T* event() const {
    return reinterpret_cast<T*>(event_);
  }

protected:
  Event* event_;
};

class ProcessorFactory {
public:
  virtual std::shared_ptr<Processor> create(Event* event) = 0;
};

} // namespace rdd
