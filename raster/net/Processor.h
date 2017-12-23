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

  virtual void run() = 0;

protected:
  Event* event_;
};

class ProcessorFactory {
public:
  virtual std::unique_ptr<Processor> create(Event* event) = 0;
};

} // namespace rdd
