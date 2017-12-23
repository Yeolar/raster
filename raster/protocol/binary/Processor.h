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
  virtual ~BinaryProcessor() {}

  virtual void process(ByteRange& response, const ByteRange& request) = 0;

  virtual void run() {
    auto transport = event_->transport<BinaryTransport>();
    try {
      ibuf_ = transport->body->coalesce();
      process(obuf_, ibuf_);
      transport->sendHeader(obuf_.size());
      transport->sendBody(IOBuf::copyBuffer(obuf_));
    } catch (std::exception& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (...) {
      RDDLOG(WARN) << "catch unknown exception";
    }
  }

private:
  ByteRange ibuf_;
  ByteRange obuf_;
};

template <class P>
class BinaryProcessorFactory : public ProcessorFactory {
public:
  BinaryProcessorFactory() {}
  virtual ~BinaryProcessorFactory() {}

  virtual std::unique_ptr<Processor> create(Event* event) {
    return make_unique<P>(event);
  }
};

} // namespace rdd
