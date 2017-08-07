/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>
#include <arpa/inet.h>
#include "raster/io/Cursor.h"
#include "raster/io/IOBuf.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Protocol.h"
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Encoding.h"

namespace rdd {

class BinaryTransport {
public:
  BinaryTransport(const Peer& peer) : peer_(peer), socket_(0) {
    rbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
    wbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  }

  virtual ~BinaryTransport() {}

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

  template <class Req>
  void send(const Req& request) {
    binary::encodeData(wbuf_.get(), (Req*)&request);
    io::Cursor cursor(wbuf_.get());
    auto p = cursor.peek();
    socket_.send((uint8_t*)p.first, p.second);
  }

  template <class Res>
  void recv(Res& response) {
    auto appender = io::Appender(rbuf_.get(), Protocol::CHUNK_SIZE);
    uint32_t n = sizeof(uint32_t);
    appender.ensure(n);
    appender.append(socket_.recv(appender.writableData(), n));
    n = ntohl(*TypedIOBuf<uint32_t>(rbuf_.get()).data());
    appender.ensure(n);
    appender.append(socket_.recv(appender.writableData(), n));
    binary::decodeData(rbuf_.get(), (Res*)&response);
  }

private:
  Peer peer_;
  Socket socket_;

  std::unique_ptr<IOBuf> rbuf_;
  std::unique_ptr<IOBuf> wbuf_;
};

} // namespace rdd
