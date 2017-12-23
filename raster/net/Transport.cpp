/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/Transport.h"

namespace rdd {

void Transport::getReadBuffer(void** buf, size_t* bufSize) {
  std::pair<void*, uint32_t> readSpace =
    readBuf_.preallocate(kMinReadSize, kMaxReadSize);
  *buf = readSpace.first;
  *bufSize = readSpace.second;
}

void Transport::readDataAvailable(size_t readSize) {
  RDDLOG(V3) << "read completed, bytes=" << readSize;
  readBuf_.postallocate(readSize);
  processReadData();
}

int Transport::readData(Socket* socket) {
  state_ = kOnReading;
  while (state_ != kFinish) {
    void* buf;
    size_t bufSize;
    getReadBuffer(&buf, &bufSize);
    ssize_t r = socket->recv(buf, bufSize);
    if (r <= 0) {
      return r;
    }
    readDataAvailable(r);
    if (state_ == kError) {
      return -1;
    }
  }
  return 1;
}

int Transport::writeData(Socket* socket) {
  if (state_ == kError) {
    return -1;
  }
  const IOBuf* buf;
  while ((buf = writeBuf_.front()) != nullptr && buf->length() != 0) {
    ssize_t r = socket->send(buf->data(), buf->length());
    if (r < 0) {
      return r;
    }
    writeBuf_.trimStart(r);
  }
  return 1;
}

} // namespace rdd
