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

#include "raster/3rd/thrift/protocol/TBinaryProtocol.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/net/AsyncClient.h"

namespace rdd {

template <class C>
class TAsyncClient : public AsyncClient {
 public:
  TAsyncClient(const ClientOption& option);

  TAsyncClient(const Peer& peer,
               const TimeoutOption& timeout);

  TAsyncClient(const Peer& peer,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000);

  ~TAsyncClient() override {}

  template <class Res>
  bool recv(void (C::*recvFunc)(Res&), Res& _return);

  template <class Res>
  bool recv(Res (C::*recvFunc)(void), Res& _return);

  template <class... Req>
  bool send(void (C::*sendFunc)(const Req&...), const Req&... requests);

  template <class Res, class... Req>
  bool fetch(void (C::*recvFunc)(Res&), Res& _return,
             void (C::*sendFunc)(const Req&...), const Req&... requests);

  template <class Res, class... Req>
  bool fetch(Res (C::*recvFunc)(void), Res& _return,
             void (C::*sendFunc)(const Req&...), const Req&... requests);

 protected:
  std::shared_ptr<Channel> makeChannel() override;

  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pibuf_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pobuf_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> poprot_;

 private:
  void init();

  std::unique_ptr<C> client_;
};

} // namespace rdd

#include "raster/protocol/thrift/AsyncClient-inl.h"
