/*
 * Copyright (C) 2018, Yeolar
 */

#include "raster/protocol/binary/Processor.h"

namespace rdd {

void BinaryProcessor::run() {
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

} // namespace rdd
