/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/http/Processor.h"
#include "raster/protocol/http/Protocol.h"

namespace rdd {

class HTTPAsyncServer : public Service {
public:
  void addRouter(const std::string& handler, const std::string& regex) {
    routers_.emplace(handler, regex);
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    std::shared_ptr<Protocol> protocol(
      new HTTPProtocol());
    std::shared_ptr<ProcessorFactory> processorFactory(
      new HTTPProcessorFactory(routers_));
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      Channel::HTTP, peer, timeoutOpt, protocol, processorFactory);
  }

private:
  std::map<std::string, std::string> routers_;
};

} // namespace rdd
