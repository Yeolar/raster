/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/protocol/binary/SyncTransport.h"

namespace rdd {

class BinarySyncClient {
 public:
  BinarySyncClient(const ClientOption& option);

  BinarySyncClient(const Peer& peer,
                   const TimeoutOption& timeout);

  BinarySyncClient(const Peer& peer,
                   uint64_t ctimeout = 100000,
                   uint64_t rtimeout = 1000000,
                   uint64_t wtimeout = 300000);

  virtual ~BinarySyncClient();

  void close();

  bool connect();

  bool connected() const;

  bool fetch(ByteRange& response, const ByteRange& request);

 private:
  void init();

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<BinarySyncTransport> transport_;
};

} // namespace rdd
