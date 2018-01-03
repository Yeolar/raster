/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/protocol/http/SyncTransport.h"

namespace rdd {

class HTTPSyncClient {
 public:
  HTTPSyncClient(const ClientOption& option);

  HTTPSyncClient(const Peer& peer,
                 const TimeoutOption& timeout);

  HTTPSyncClient(const Peer& peer,
                 uint64_t ctimeout = 100000,
                 uint64_t rtimeout = 1000000,
                 uint64_t wtimeout = 300000);

  virtual ~HTTPSyncClient();

  void close();

  bool connect();

  bool connected() const;

  bool fetch(const HTTPMessage& headers, std::unique_ptr<IOBuf> body);

  HTTPMessage* headers() const;
  IOBuf* body() const;
  HTTPHeaders* trailers() const;

 private:
  void init();

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<HTTPSyncTransport> transport_;
};

} // namespace rdd
