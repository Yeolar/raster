/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/IOBuf.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class BinarySyncTransport : public BinaryTransport {
public:
  BinarySyncTransport(const Peer& peer)
    : BinaryTransport(),
      peer_(peer) {}

  virtual ~BinarySyncTransport() {}

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

  void send(const ByteRange& request) {
    sendHeader(request.size());
    sendBody(IOBuf::copyBuffer(request));
    writeData(socket_.get());
  }

  void recv(ByteRange& response) {
    readData(socket_.get());
    response = body->coalesce();
  }

private:
  Peer peer_;
  std::unique_ptr<Socket> socket_;
};

} // namespace rdd
