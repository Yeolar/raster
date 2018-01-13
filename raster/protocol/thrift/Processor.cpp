/*
 * Copyright (C) 2018, Yeolar
 */

#include "raster/protocol/thrift/Processor.h"

#include "raster/protocol/binary/Transport.h"

namespace rdd {

TProcessor::TProcessor(
    Event* event,
    std::unique_ptr< ::apache::thrift::TProcessor> processor)
  : Processor(event), processor_(std::move(processor)) {
  pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
  poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));
}

void TProcessor::run() {
  auto transport = event_->transport<BinaryTransport>();
  try {
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());
    processor_->process(piprot_, poprot_, nullptr);
    uint8_t* p;
    uint32_t n;
    pobuf_->getBuffer(&p, &n);
    transport->sendHeader(n);
    transport->sendBody(IOBuf::copyBuffer(p, n));;
  } catch (apache::thrift::protocol::TProtocolException& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (std::exception& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    RDDLOG(WARN) << "catch unknown exception";
  }
}

void TZlibProcessor::run() {
  auto transport = event_->transport<ZlibTransport>();
  try {
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());
    processor_->process(piprot_, poprot_, nullptr);
    uint8_t* p;
    uint32_t n;
    pobuf_->getBuffer(&p, &n);
    transport->sendBody(IOBuf::copyBuffer(p, n));;
  } catch (apache::thrift::protocol::TProtocolException& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (std::exception& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    RDDLOG(WARN) << "catch unknown exception";
  }
}

} // namespace rdd
