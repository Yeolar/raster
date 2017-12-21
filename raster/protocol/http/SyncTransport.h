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
      peer_(peer),
      socket_(0) {
    rbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
    wbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  }

  virtual ~HTTPSyncTransport() {}

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

  void send() {
    pushWriteData(wbuf_.get());
    io::Cursor cursor(wbuf_.get());
    auto p = cursor.peek();
    socket_.send((uint8_t*)p.first, p.second);
  }

  void recv() {
    /* TODO
    io::Appender appender(rbuf_.get(), Protocol::CHUNK_SIZE);
    uint32_t n = sizeof(uint32_t);
    appender.ensure(n);
    appender.append(socket_.recv(appender.writableData(), n));
    n = ntohl(*TypedIOBuf<uint32_t>(rbuf_.get()).data());
    appender.ensure(n);
    appender.append(socket_.recv(appender.writableData(), n));
  */
    parseReadData(rbuf_.get());
  }

private:
  Peer peer_;
  Socket socket_;

  std::unique_ptr<IOBuf> rbuf_;
  std::unique_ptr<IOBuf> wbuf_;
};

} // namespace rdd
