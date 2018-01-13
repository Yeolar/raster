/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
