/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/AsyncServer.h"

namespace rdd {

void HTTPAsyncServer::makeChannel(int port, const TimeoutOption& timeoutOpt) {
  Peer peer;
  peer.setFromLocalPort(port);
  channel_ = std::make_shared<Channel>(
      peer,
      timeoutOpt,
      make_unique<HTTPTransportFactory>(TransportDirection::DOWNSTREAM),
      make_unique<HTTPProcessorFactory>(routers_));
}

void HTTPAsyncServer::addRouter(const std::string& handler,
                                const std::string& regex) {
  routers_.emplace(handler, regex);
}

} // namespace rdd
