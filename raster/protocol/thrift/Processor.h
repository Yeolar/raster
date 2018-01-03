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

namespace rdd {

class TProcessor : public Processor {
 public:
  TProcessor(
      Event* event,
      std::unique_ptr< ::apache::thrift::TProcessor> processor);

  ~TProcessor() override {}

  void run() override;

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

  ~TZlibProcessor() override {}

  void run() override;
};

template <class P, class If, class ProcessorType>
class TProcessorFactory : public ProcessorFactory {
 public:
  TProcessorFactory() : handler_(new If()) {}
  ~TProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override {
    return make_unique<ProcessorType>(event, make_unique<P>(handler_));
  }

  If* handler() { return handler_.get(); }

 private:
  boost::shared_ptr<If> handler_;
};

} // namespace rdd
