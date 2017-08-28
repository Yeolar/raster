/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Protocol.h"
#include "raster/protocol/http/HTTPEvent.h"

namespace rdd {

// protocol of HTTP
class HTTPProtocol : public Protocol {
public:
  HTTPProtocol() : Protocol() {}
  virtual ~HTTPProtocol() {}

  virtual int readData(Event* event) {
    HTTPEvent* http_event = dynamic_cast<HTTPEvent*>(event);
    IOBuf* buf = http_event->rbuf();
    int r = 0;
    if (http_event->readingHeaders) {
      r = Protocol::readDataUntil(
          event, ByteRange((uint8_t*)"\r\n\r\n", 4));
      if (r == 0) {
        http_event->readingHeaders = false;
        http_event->parse();
      }
    } else {
      size_t n = http_event->bodyLength();
      if (n > BODYLEN_LIMIT) {
        RDDLOG(WARN) << *event << " big request, bodyLength=" << n;
      }
      event->rlen() = n;
      r = Protocol::readData(event);
    }
    return r;
  }
};

}
