/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/binary/Processor.h"
#include "raster/protocol/binary/Protocol.h"

namespace rdd {

template <class P>
class BinaryAsyncServer : public Service {
public:
  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    std::shared_ptr<Protocol> protocol(
      new BinaryProtocol());
    std::shared_ptr<ProcessorFactory> processorFactory(
      new BinaryProcessorFactory<P>());
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      Channel::DEFAULT, peer, timeoutOpt, protocol, processorFactory);
  }
};

} // namespace rdd
