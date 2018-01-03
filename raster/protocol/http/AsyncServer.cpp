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
      make_unique<HTTPProcessorFactory>(
          [&](const std::string& url) {
            return matchHandler(url);
          }));
}

std::shared_ptr<RequestHandler>
HTTPAsyncServer::matchHandler(const std::string& url) const {
  for (auto& kv : handlers_) {
    boost::cmatch match;
    if (boost::regex_match(url.c_str(), match, kv.first)) {
      return kv.second;
    }
  }
  return std::make_shared<RequestHandler>();
}

} // namespace rdd
