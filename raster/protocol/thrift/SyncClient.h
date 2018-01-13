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

#include <arpa/inet.h>

#include "raster/3rd/thrift/transport/TSocket.h"
#include "raster/3rd/thrift/transport/TTransport.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/3rd/thrift/protocol/TBinaryProtocol.h"
#include "raster/net/NetUtil.h"

namespace rdd {

template <
  class C,
  class TTransport = apache::thrift::transport::TFramedTransport,
  class TProtocol = apache::thrift::protocol::TBinaryProtocol>
class TSyncClient {
 public:
  TSyncClient(const ClientOption& option);

  TSyncClient(const Peer& peer,
              const TimeoutOption& timeout);

  TSyncClient(const Peer& peer,
              uint64_t ctimeout = 100000,
              uint64_t rtimeout = 1000000,
              uint64_t wtimeout = 300000);

  virtual ~TSyncClient();

  void close();

  bool connect();

  bool connected() const;

  template <class Res, class... Req>
  bool fetch(void (C::*func)(Res&, const Req&...),
             Res& response, const Req&... requests);

  template <class Res, class... Req>
  bool fetch(Res (C::*func)(const Req&...),
             Res& response, const Req&... requests);

 private:
  void init();

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<C> client_;
  boost::shared_ptr<apache::thrift::transport::TSocket> socket_;
  boost::shared_ptr<apache::thrift::transport::TTransport> transport_;
  boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol_;
};

} // namespace rdd

#include "raster/protocol/thrift/SyncClient-inl.h"
