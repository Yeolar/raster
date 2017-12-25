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
  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    Peer peer;
    peer.setFromLocalPort(port);
    channel_ = std::make_shared<Channel>(
        peer,
        timeoutOpt,
        make_unique<BinaryTransportFactory>(),
        make_unique<BinaryProcessorFactory<P>>());
  }
};

} // namespace rdd
