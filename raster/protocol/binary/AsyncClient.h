/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/AsyncClient.h"

namespace rdd {

class BinaryAsyncClient : public AsyncClient {
 public:
  BinaryAsyncClient(const ClientOption& option);

  BinaryAsyncClient(const Peer& peer,
                    const TimeoutOption& timeout);

  BinaryAsyncClient(const Peer& peer,
                    uint64_t ctimeout = 100000,
                    uint64_t rtimeout = 1000000,
                    uint64_t wtimeout = 300000);

  ~BinaryAsyncClient() override {}

  bool recv(ByteRange& response);

  bool send(const ByteRange& request);

  bool fetch(ByteRange& response, const ByteRange& request);

 protected:
  std::shared_ptr<Channel> makeChannel() override;
};

} // namespace rdd
