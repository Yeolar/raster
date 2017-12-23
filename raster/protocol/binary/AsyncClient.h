/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/AsyncClient.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class BinaryAsyncClient : public AsyncClient {
public:
  BinaryAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    channel_ = makeChannel();
  }
  BinaryAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : BinaryAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~BinaryAsyncClient() {}

  bool recv(ByteRange& _return) {
    if (!event_ || event_->state() == Event::kFail) {
      return false;
    }
    auto transport = event_->transport<BinaryTransport>();
    _return = transport->body->coalesce();
    return true;
  }

  bool send(const ByteRange& request) {
    if (!event_) {
      return false;
    }
    auto transport = event_->transport<BinaryTransport>();
    transport->sendHeader(request.size());
    transport->sendBody(IOBuf::copyBuffer(request));
    return true;
  }

  bool fetch(ByteRange& _return, const ByteRange& request) {
    return (send(request) &&
            FiberManager::yield() &&
            recv(_return));
  }

  bool fetchNoWait(const ByteRange& request) {
    if (send(request)) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    return std::make_shared<Channel>(
        peer_,
        timeoutOpt_,
        make_unique<BinaryTransportFactory>());
  }
};

} // namespace rdd
