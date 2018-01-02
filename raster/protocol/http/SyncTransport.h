/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/protocol/http/Transport.h"

namespace rdd {

class HTTPSyncTransport : public HTTPTransport {
 public:
  HTTPSyncTransport(const Peer& peer, const TimeoutOption& timeout)
    : HTTPTransport(TransportDirection::UPSTREAM),
      peer_(peer),
      timeout_(timeout) {}

  ~HTTPSyncTransport() override {}

  void open();

  bool isOpen();

  void close();

  void send();

  void recv();

 private:
  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<Socket> socket_;
};

} // namespace rdd
