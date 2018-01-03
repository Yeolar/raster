/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/SyncTransport.h"

namespace rdd {

void HTTPSyncTransport::open() {
  socket_ = Socket::createSyncSocket();
  socket_->setConnTimeout(timeout_.ctimeout);
  socket_->setRecvTimeout(timeout_.rtimeout);
  socket_->setSendTimeout(timeout_.wtimeout);
  socket_->connect(peer_);
}

bool HTTPSyncTransport::isOpen() {
  return socket_->isConnected();
}

void HTTPSyncTransport::close() {
  socket_->close();
}

void HTTPSyncTransport::send() {
  writeData(socket_.get());
}

void HTTPSyncTransport::recv() {
  readData(socket_.get());
}

} // namespace rdd
