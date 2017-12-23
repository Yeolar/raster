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
      peer_(peer),
      socket_(0) {}

  virtual ~BinarySyncTransport() {}

  void open() {
    socket_.setReuseAddr();
    socket_.setTCPNoDelay();
    socket_.connect(peer_);
  }

  bool isOpen() {
    return socket_.isConnected();
  }

  void close() {
    socket_.close();
  }

  void send(const ByteRange& request) {
    sendHeader(request.size());
    sendBody(IOBuf::copyBuffer(request));
    writeData(&socket_);
  }

  void recv(ByteRange& response) {
    readData(&socket_);
    response = body->coalesce();
  }

private:
  Peer peer_;
  Socket socket_;
};

} // namespace rdd
