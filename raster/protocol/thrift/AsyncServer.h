/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/thrift/Processor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

template <class If, class P>
class TAsyncServer : public Service {
 public:
  TAsyncServer(StringPiece name) : Service(name) {}
  ~TAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeout) override {
    Peer peer;
    peer.setFromLocalPort(port);
    channel_ = std::make_shared<Channel>(
        peer,
        timeout,
        make_unique<BinaryTransportFactory>(),
        make_unique<TProcessorFactory<P, If, TProcessor>>());
  }

  If* handler() {
    auto pf = channel_->processorFactory();
    return ((TProcessorFactory<P, If, TProcessor>*)(pf))->handler();
  }
};

template <class If, class P>
class TZlibAsyncServer : public Service {
 public:
  TZlibAsyncServer(StringPiece name) : Service(name) {}
  ~TZlibAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeout) override {
    Peer peer;
    peer.setFromLocalPort(port);
    channel_ = std::make_shared<Channel>(
        peer,
        timeout,
        make_unique<ZlibTransportFactory>(),
        make_unique<TProcessorFactory<P, If, TZlibProcessor>>());
  }

  If* handler() {
    auto pf = channel_->processorFactory();
    return ((TProcessorFactory<P, If, TZlibProcessor>*)(pf))->handler();
  }
};

} // namespace rdd
