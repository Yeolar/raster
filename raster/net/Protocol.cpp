/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Cursor.h"
#include "raster/net/Protocol.h"

namespace rdd {

int Protocol::readData(Event* event) {
  io::Appender appender(event->rbuf.get(), CHUNK_SIZE);
  appender.ensure(event->rlen);
  while (event->rlen > 0) {
    ssize_t r = event->socket()->recv(appender.writableData(), event->rlen);
    if (r <= 0) return r;
    appender.append(r);
    event->rlen -= r;
  }
  return 1;
}

int Protocol::readDataUntil(Event* event, ByteRange bytes) {
  io::Appender appender(event->rbuf.get(), CHUNK_SIZE);
  while (true) {
    if (appender.length() == 0) {
      appender.ensure(CHUNK_SIZE);
    }
    ssize_t r = event->socket()->recv(appender.writableData(), appender.length());
    if (r <= 0) return r;
    appender.append(r);
    if (event->rbuf->coalesce().find(bytes) != ByteRange::npos) {
      break;
    }
  }
  return 1;
}

int Protocol::writeData(Event* event) {
  rdd::io::Cursor cursor(event->wbuf.get());
  cursor += event->wlen;
  while (!cursor.isAtEnd()) {
    auto p = cursor.peek();
    ssize_t r = event->socket()->send((uint8_t*)p.first, p.second);
    if (r < 0) return r;
    cursor += r;
    event->wlen += r;
  }
  return 1;
}

} // namespace rdd
