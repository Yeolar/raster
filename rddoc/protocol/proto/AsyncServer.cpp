/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/protocol/proto/AsyncServer.h"
#include "rddoc/protocol/proto/Processor.h"

namespace rdd {

void PBAsyncServer::makeChannel(int port, const TimeoutOption& timeout_opt) {
  std::shared_ptr<Protocol> protocol(
    new PBBinaryProtocol());
  std::shared_ptr<ProcessorFactory> processor_factory(
    new PBProcessorFactory(this));
  Peer peer = {"", port};
  channel_ = std::make_shared<Channel>(
    peer, timeout_opt, protocol, processor_factory);
}

}
