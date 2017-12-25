/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/Cursor.h"
#include "raster/net/Protocol.h"
#include "raster/protocol/http/Transport.h"

namespace rdd {

class HTTPSyncTransport : public HTTPTransport {
public:
  HTTPSyncTransport(const Peer& peer)
    : HTTPTransport(TransportDirection::UPSTREAM),
      peer_(peer) {
  }

  virtual ~HTTPSyncTransport() {}

  void open() {
    socket_ = Socket::createSyncSocket();
    socket_->connect(peer_);
  }

  bool isOpen() {
    return socket_->isConnected();
  }

  void close() {
    socket_->close();
  }

  void send() {
    writeData(socket_.get());
  }

  void recv() {
    readData(socket_.get());
  }

private:
  Peer peer_;
  std::unique_ptr<Socket> socket_;
};

} // namespace rdd
