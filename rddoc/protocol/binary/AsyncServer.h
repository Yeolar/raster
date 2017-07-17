/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Service.h"
#include "rddoc/protocol/binary/Processor.h"
#include "rddoc/protocol/binary/Protocol.h"

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
      peer, timeoutOpt, protocol, processorFactory);
  }
};

}

