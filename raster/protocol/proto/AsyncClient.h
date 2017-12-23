/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/AsyncClient.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/proto/RpcChannel.h"
#include "raster/protocol/proto/RpcController.h"

namespace rdd {

template <class C>
class PBAsyncClient : public AsyncClient {
public:
  PBAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    rpcChannel_.reset(new PBAsyncRpcChannel(event()));
    controller_.reset(new PBRpcController());
    service_.reset(new C(rpcChannel_.get()));
    channel_ = makeChannel();
  }
  PBAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : PBAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~PBAsyncClient() {}

  bool recv() {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    auto transport = event_->transport<BinaryTransport>();
    rpcChannel_->process(transport->body);
    return true;
  }

  template <class Res, class Req>
  bool send(void (C::*func)(google::protobuf::RpcController*,
                            const Req*, Res*,
                            google::protobuf::Closure*),
            Res& _return, const Req& request) {
    if (!event_) {
      return false;
    }
    (service_.get()->*func)(controller_.get(), &request, &_return, nullptr);
    return true;
  }

  template <class Res, class Req>
  bool fetch(void (C::*func)(google::protobuf::RpcController*,
                             const Req*, Res*,
                             google::protobuf::Closure*),
             Res& _return, const Req& request) {
    return (send(func, _return, request) &&
            FiberManager::yield() &&
            recv());
  }

  template <class Res, class Req>
  bool fetchNoWait(void (C::*func)(google::protobuf::RpcController*,
                                   const Req*, Res*,
                                   google::protobuf::Closure*),
                   Res& _return, const Req& request) {
    if (send(func, _return, request)) {
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

private:
  std::shared_ptr<PBAsyncRpcChannel> rpcChannel_;
  std::shared_ptr<google::protobuf::RpcController> controller_;
  std::shared_ptr<C> service_;
};

} // namespace rdd
