/*
 * Copyright (C) 2017, Yeolar
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
