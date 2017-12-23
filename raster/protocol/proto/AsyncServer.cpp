/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/proto/AsyncServer.h"
#include "raster/protocol/proto/Processor.h"

namespace rdd {

void PBAsyncServer::makeChannel(int port, const TimeoutOption& timeout_opt) {
  Peer peer = {"", port};
  channel_ = std::make_shared<Channel>(
      peer,
      timeout_opt,
      make_unique<BinaryTransportFactory>(),
      make_unique<PBProcessorFactory>(this));
}

} // namespace rdd
