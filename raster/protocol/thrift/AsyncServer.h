/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/thrift/Processor.h"
#include "raster/protocol/thrift/Protocol.h"

namespace rdd {

template <class If, class P>
class TAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((ThriftProcessorFactory<P, If, ThriftProcessor>*)(pf.get()))
      ->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    std::shared_ptr<Protocol> protocol(
      new TFramedProtocol());
    std::shared_ptr<ProcessorFactory> processorFactory(
      new ThriftProcessorFactory<P, If, ThriftProcessor>());
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      peer, timeoutOpt, protocol, processorFactory);
  }
};

template <class If, class P>
class TZlibAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((ThriftProcessorFactory<P, If, ThriftZlibProcessor>*)(pf.get()))
      ->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    std::shared_ptr<Protocol> protocol(
      new TZlibProtocol());
    std::shared_ptr<ProcessorFactory> processorFactory(
      new ThriftProcessorFactory<P, If, ThriftZlibProcessor>());
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      peer, timeoutOpt, protocol, processorFactory);
  }
};

} // namespace rdd
