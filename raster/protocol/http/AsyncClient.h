/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>

#include "raster/net/AsyncClient.h"
#include "raster/protocol/http/Protocol.h"
#include "raster/protocol/http/Transport.h"
#include "raster/util/Logging.h"
#include "raster/util/ScopeGuard.h"

namespace rdd {

class HTTPAsyncClient : public AsyncClient {
public:
  HTTPAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    channel_ = makeChannel();
  }
  HTTPAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : HTTPAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~HTTPAsyncClient() {}

  bool recv() {
    if (!event_ || event_->state() == Event::kFail) {
      return false;
    }
    return true;
  }

  bool send(const HTTPMessage& headers, std::unique_ptr<IOBuf> body) {
    if (!event_) {
      return false;
    }
    auto transport = event_->transport<HTTPTransport>();
    transport->sendHeaders(headers, nullptr);
    transport->sendBody(std::move(body), false);
    transport->sendEOM();
    return true;
  }

  bool fetch(const HTTPMessage& headers, std::unique_ptr<IOBuf> body) {
    return (send(headers, std::move(body)) &&
            FiberManager::yield() &&
            recv());
  }

  bool fetchNoWait(const HTTPMessage& headers, std::unique_ptr<IOBuf> body) {
    if (send(headers, std::move(body))) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

  HTTPMessage* message() const {
    return event_->transport<HTTPTransport>()->headers.get();
  }
  IOBuf* body() const {
    return event_->transport<HTTPTransport>()->body.get();
  }
  HTTPHeaders* trailers() const {
    return event_->transport<HTTPTransport>()->trailers.get();
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    return std::make_shared<Channel>(
        peer_,
        timeoutOpt_,
        make_unique<HTTPTransportFactory>(TransportDirection::UPSTREAM));
  }
};

} // namespace rdd
