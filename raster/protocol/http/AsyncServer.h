/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/http/Processor.h"

namespace rdd {

class HTTPAsyncServer : public Service {
public:
  void addRouter(const std::string& handler, const std::string& regex) {
    routers_.emplace(handler, regex);
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) {
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
        peer,
        timeoutOpt,
        make_unique<HTTPTransportFactory>(TransportDirection::DOWNSTREAM),
        make_unique<HTTPProcessorFactory>(routers_));
  }

private:
  std::map<std::string, std::string> routers_;
};

} // namespace rdd
