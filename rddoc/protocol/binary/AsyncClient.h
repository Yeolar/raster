/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/AsyncClient.h"
#include "rddoc/protocol/binary/Encoding.h"
#include "rddoc/protocol/binary/Protocol.h"

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

  template <class Res = std::vector<uint8_t>>
  bool recv(Res& _return) {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    binary::decodeData(event_->rbuf(), &_return);
    return true;
  }

  template <class Req = std::vector<uint8_t>>
  bool send(const Req& request) {
    if (!event_) {
      return false;
    }
    binary::encodeData(event_->wbuf(), (Req*)&request);
    return true;
  }

  template <class Req = std::vector<uint8_t>,
            class Res = std::vector<uint8_t>>
  bool fetch(Res& _return, const Req& request) {
    return (send(request) &&
            FiberManager::yield() &&
            recv(_return));
  }

  template <class Req = std::vector<uint8_t>>
  bool fetchNoWait(const Req& request) {
    if (send(request)) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    std::shared_ptr<Protocol> protocol(new BinaryProtocol());
    return std::make_shared<Channel>(peer_, timeoutOpt_, protocol);
  }
};

}

