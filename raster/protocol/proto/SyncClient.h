/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <google/protobuf/service.h>

#include "raster/net/NetUtil.h"
#include "raster/protocol/proto/RpcChannel.h"
#include "raster/protocol/proto/RpcController.h"

namespace rdd {

template <class C>
class PBSyncClient {
 public:
  PBSyncClient(const ClientOption& option);

  PBSyncClient(const Peer& peer,
               const TimeoutOption& timeout);

  PBSyncClient(const Peer& peer,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000);

  virtual ~PBSyncClient();

  void close();

  bool connect();

  bool connected() const;

  template <class Res, class Req>
  bool fetch(void (C::*func)(google::protobuf::RpcController*,
                             const Req*, Res*,
                             google::protobuf::Closure*),
             Res& _return,
             const Req& request);

 private:
  void init();

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<PBSyncRpcChannel> rpcChannel_;
  std::unique_ptr<google::protobuf::RpcController> controller_;
  std::unique_ptr<C> service_;
};

} // namespace rdd

#include "raster/protocol/proto/SyncClient-inl.h"
