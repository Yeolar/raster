/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/AsyncClient.h"
#include "raster/protocol/proto/RpcChannel.h"

namespace rdd {

template <class C>
class PBAsyncClient : public AsyncClient {
 public:
  PBAsyncClient(const ClientOption& option);

  PBAsyncClient(const Peer& peer,
                const TimeoutOption& timeout);

  PBAsyncClient(const Peer& peer,
                uint64_t ctimeout = 100000,
                uint64_t rtimeout = 1000000,
                uint64_t wtimeout = 300000);

  ~PBAsyncClient() override {}

  bool recv();

  template <class Res, class Req>
  bool send(void (C::*func)(google::protobuf::RpcController*,
                            const Req*, Res*,
                            google::protobuf::Closure*),
            Res& response,
            const Req& request);

  template <class Res, class Req>
  bool fetch(void (C::*func)(google::protobuf::RpcController*,
                             const Req*, Res*,
                             google::protobuf::Closure*),
             Res& response,
             const Req& request);

 protected:
  virtual std::shared_ptr<Channel> makeChannel();

 private:
  void init();

  std::unique_ptr<PBAsyncRpcChannel> rpcChannel_;
  std::unique_ptr<google::protobuf::RpcController> controller_;
  std::unique_ptr<C> service_;
};

} // namespace rdd

#include "raster/protocol/proto/AsyncClient-inl.h"
