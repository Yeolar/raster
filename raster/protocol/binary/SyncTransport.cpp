/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/binary/SyncTransport.h"

#include "raster/io/IOBuf.h"

namespace rdd {

void BinarySyncTransport::open() {
  socket_ = Socket::createSyncSocket();
  socket_->setConnTimeout(timeout_.ctimeout);
  socket_->setRecvTimeout(timeout_.rtimeout);
  socket_->setSendTimeout(timeout_.wtimeout);
  socket_->connect(peer_);
}

bool BinarySyncTransport::isOpen() {
  return socket_->isConnected();
}

void BinarySyncTransport::close() {
  socket_->close();
}

void BinarySyncTransport::send(const ByteRange& request) {
  sendHeader(request.size());
  sendBody(IOBuf::copyBuffer(request));
  writeData(socket_.get());
}

void BinarySyncTransport::recv(ByteRange& response) {
  readData(socket_.get());
  response = body->coalesce();
}

} // namespace rdd
