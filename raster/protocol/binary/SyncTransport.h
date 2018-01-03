/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class BinarySyncTransport : public BinaryTransport {
 public:
  BinarySyncTransport(const Peer& peer, const TimeoutOption& timeout)
    : BinaryTransport(),
      peer_(peer),
      timeout_(timeout) {}

  ~BinarySyncTransport() override {}

  void open();

  bool isOpen();

  void close();

  void send(const ByteRange& request);

  void recv(ByteRange& response);

 private:
  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<Socket> socket_;
};

} // namespace rdd
