/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/binary/Processor.h"

namespace rdd {

template <class P>
class BinaryAsyncServer : public Service {
 public:
  BinaryAsyncServer(StringPiece name) : Service(name) {}
  ~BinaryAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeout) override {
    Peer peer;
    peer.setFromLocalPort(port);
    channel_ = std::make_shared<Channel>(
        peer,
        timeout,
        make_unique<BinaryTransportFactory>(),
        make_unique<BinaryProcessorFactory<P>>());
  }
};

} // namespace rdd
