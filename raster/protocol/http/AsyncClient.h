/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/AsyncClient.h"
#include "raster/protocol/http/HTTPHeaders.h"
#include "raster/protocol/http/HTTPMessage.h"

namespace rdd {

class HTTPAsyncClient : public AsyncClient {
 public:
  HTTPAsyncClient(const ClientOption& option);

  HTTPAsyncClient(const Peer& peer,
                  const TimeoutOption& timeout);

  HTTPAsyncClient(const Peer& peer,
                  uint64_t ctimeout = 100000,
                  uint64_t rtimeout = 1000000,
                  uint64_t wtimeout = 300000);

  ~HTTPAsyncClient() override {}

  bool recv();

  bool send(const HTTPMessage& headers, std::unique_ptr<IOBuf> body);

  bool fetch(const HTTPMessage& headers, std::unique_ptr<IOBuf> body);

  HTTPMessage* message() const;
  IOBuf* body() const;
  HTTPHeaders* trailers() const;

 protected:
  std::shared_ptr<Channel> makeChannel() override;
};

} // namespace rdd
