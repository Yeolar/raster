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

  void setHTTPVersionOverride(bool enabled) {
    forceHTTP1xCodecTo1_1_ = enabled;
  }

  virtual void onConnect() {
    codec_ = make_unique<HTTP1xCodec>(TransportDirection::UPSTREAM,
                                      forceHTTP1xCodecTo1_1_);
  }

  bool recv() {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    return true;
  }

  bool send(const HTTPMessage& headers, std::unique_ptr<IOBuf> body) {
    if (!event_) {
      return false;
    }
    transport()->sendHeaders(headers, nullptr);
    transport()->sendBody(std::move(body), false);
    transport()->sendEOM();
    event<HTTPEvent>()->pushWriteData();
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

  HTTPMessage* message() const { return event<HTTPEvent>()->message(); }
  IOBuf* body() const { return event<HTTPEvent>()->body(); }
  HTTPHeaders* trailers() const { return event<HTTPEvent>()->trailers(); }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    std::shared_ptr<Protocol> protocol(new HTTPProtocol());
    return std::make_shared<Channel>(
        Channel::HTTP, peer_, timeoutOpt_, protocol);
  }

private:
  HTTPTransport* transport() const {
    return event<HTTPTransport>();
  }

  std::unique_ptr<HTTP1xCodec> codec_;
  bool forceHTTP1xCodecTo1_1_;
};

} // namespace rdd
