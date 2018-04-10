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
    transport->sendBody(acc::IOBuf::copyBuffer(obuf_));
  } catch (std::exception& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    ACCLOG(WARN) << "catch unknown exception";
  }
}

} // namespace rdd
