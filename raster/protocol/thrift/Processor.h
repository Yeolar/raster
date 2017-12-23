/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>
#include <boost/shared_ptr.hpp>

#include "raster/3rd/thrift/TProcessor.h"
#include "raster/3rd/thrift/protocol/TBinaryProtocol.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/3rd/thrift/transport/TTransportException.h"
#include "raster/net/Processor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class TProcessor : public Processor {
public:
  TProcessor(
      Event* event,
      std::unique_ptr< ::apache::thrift::TProcessor> processor)
    : Processor(event), processor_(std::move(processor)) {
    pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
    poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));
  }

  virtual ~TProcessor() {}

  virtual void run() {
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

protected:
  std::unique_ptr< ::apache::thrift::TProcessor> processor_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pibuf_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pobuf_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> poprot_;
};

class TZlibProcessor : public TProcessor {
public:
  TZlibProcessor(
      Event* event,
      std::unique_ptr< ::apache::thrift::TProcessor> processor)
    : TProcessor(event, std::move(processor)) {}

  virtual ~TZlibProcessor() {}

  virtual void run() {
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

};

template <class P, class If, class ProcessorType>
class TProcessorFactory : public ProcessorFactory {
public:
  TProcessorFactory() : handler_(new If()) {}
  virtual ~TProcessorFactory() {}

  virtual std::unique_ptr<Processor> create(Event* event) {
    return make_unique<ProcessorType>(event, make_unique<P>(handler_));
  }

  If* handler() { return handler_.get(); }

private:
  boost::shared_ptr<If> handler_;
};

} // namespace rdd
