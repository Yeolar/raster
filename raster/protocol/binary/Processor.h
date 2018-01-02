/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Processor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class BinaryProcessor : public Processor {
 public:
  BinaryProcessor(Event* event) : Processor(event) {}
  ~BinaryProcessor() override {}

  virtual void process(ByteRange& response, const ByteRange& request) = 0;

  void run() override;

 private:
  ByteRange ibuf_;
  ByteRange obuf_;
};

template <class P>
class BinaryProcessorFactory : public ProcessorFactory {
 public:
  BinaryProcessorFactory() {}
  ~BinaryProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override {
    return make_unique<P>(event);
  }
};

} // namespace rdd
