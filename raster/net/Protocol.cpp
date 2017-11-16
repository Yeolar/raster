/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Cursor.h"
#include "raster/net/Protocol.h"

namespace rdd {

int Protocol::readData(Event* event) {
  io::Appender appender(event->rbuf().get(), CHUNK_SIZE);
  appender.ensure(event->rlen());
  while (event->rlen() > 0) {
    int r = event->socket()->recv(appender.writableData(), event->rlen());
    if (r < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        if (event->isReadTimeout()) {
          event->setType(Event::TIMEOUT);
          return -1;
        }
        return 1;
      }
      if (errno == ECONNRESET) {
        return 2;
      }
      return -1;
    }
    if (r == 0) {
      return 2;
    }
    appender.append(r);
    event->rlen() -= r;
  }
  return 0;
}

int Protocol::readDataUntil(Event* event, ByteRange bytes) {
  io::Appender appender(event->rbuf().get(), CHUNK_SIZE);
  while (true) {
    if (appender.length() == 0) {
      appender.ensure(CHUNK_SIZE);
    }
    int r = event->socket()->recv(appender.writableData(), appender.length());
    if (r < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        if (event->isReadTimeout()) {
          event->setType(Event::TIMEOUT);
          return -1;
        }
        return 1;
      }
      if (errno == ECONNRESET) {
        return 2;
      }
      return -1;
    }
    if (r == 0) {
      return 2;
    }
    appender.append(r);
    if (event->rbuf()->coalesce().find(bytes) != ByteRange::npos) {
      break;
    }
  }
  return 0;
}

int Protocol::writeData(Event* event) {
  rdd::io::Cursor cursor(event->wbuf().get());
  cursor += event->wlen();
  while (!cursor.isAtEnd()) {
    auto p = cursor.peek();
    int r = event->socket()->send((uint8_t*)p.first, p.second);
    if (r < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        if (event->isWriteTimeout()) {
          event->setType(Event::TIMEOUT);
          return -1;
        }
        return 1;
      }
      return -1;
    }
    cursor += r;
    event->wlen() += r;
  }
  return 0;
}

} // namespace rdd
