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
#include "raster/io/event/Event.h"
#include "raster/net/Processor.h"
#include "raster/protocol/thrift/Encoding.h"

namespace rdd {

class ThriftProcessor : public Processor {
public:
  ThriftProcessor(
      Event* event,
      const std::shared_ptr< ::apache::thrift::TProcessor>& processor)
    : Processor(event), processor_(processor) {
    pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
    poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));
  }

  virtual ~ThriftProcessor() {}

  virtual bool decodeData() {
    return rdd::thrift::decodeData(event_->rbuf, pibuf_.get());
  }

  virtual bool encodeData() {
    return rdd::thrift::encodeData(event_->wbuf, pobuf_.get());
  }

  virtual bool run() {
    try {
      return processor_->process(piprot_, poprot_, nullptr);
    } catch (apache::thrift::protocol::TProtocolException& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (std::exception& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (...) {
      RDDLOG(WARN) << "catch unknown exception";
    }
    return false;
  }

protected:
  std::shared_ptr< ::apache::thrift::TProcessor> processor_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pibuf_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pobuf_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> poprot_;
};

class ThriftZlibProcessor : public ThriftProcessor {
public:
  ThriftZlibProcessor(
      Event* event,
      const std::shared_ptr< ::apache::thrift::TProcessor>& processor)
    : ThriftProcessor(event, processor) {}

  virtual ~ThriftZlibProcessor() {}

  virtual bool decodeData() {
    return rdd::thrift::decodeZlibData(event_->rbuf, pibuf_.get());
  }

  virtual bool encodeData() {
    return rdd::thrift::encodeZlibData(event_->wbuf, pobuf_.get());
  }
};

template <class P, class If, class ProcessorType>
class ThriftProcessorFactory : public ProcessorFactory {
public:
  ThriftProcessorFactory() : handler_(new If()) {}
  virtual ~ThriftProcessorFactory() {}

  virtual std::shared_ptr<Processor> create(Event* event) {
    return std::shared_ptr<Processor>(
      new ProcessorType(
          event,
          std::shared_ptr< ::apache::thrift::TProcessor>(new P(handler_))));
  }

  If* handler() { return handler_.get(); }

private:
  boost::shared_ptr<If> handler_;
};

} // namespace rdd
