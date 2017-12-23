/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/thrift/Processor.h"

namespace rdd {

template <class If, class P>
class TAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((TProcessorFactory<P, If, TProcessor>*)(pf))->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
        peer,
        timeoutOpt,
        make_unique<BinaryTransportFactory>(),
        make_unique<TProcessorFactory<P, If, TProcessor>>());
  }
};

template <class If, class P>
class TZlibAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((TProcessorFactory<P, If, TZlibProcessor>*)(pf))->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
        peer,
        timeoutOpt,
        make_unique<ZlibTransportFactory>(),
        make_unique<TProcessorFactory<P, If, TZlibProcessor>>());
  }
};

} // namespace rdd
