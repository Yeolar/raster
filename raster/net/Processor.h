/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>

namespace rdd {

class Event;

class Processor {
public:
  virtual bool decodeData(Event* event) = 0;
  virtual bool encodeData(Event* event) = 0;
  virtual bool run() = 0;
};

class ProcessorFactory {
public:
  virtual std::shared_ptr<Processor> create(Event* event) = 0;
};

} // namespace rdd
