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
    static const ByteRange kEnd((uint8_t*)"\r\n\r\n", 4);
    HTTPEvent* httpev = reinterpret_cast<HTTPEvent*>(event);
    auto state = httpev->transport.state();
    if (state == HTTPTransport::kInit) {
      int r = Protocol::readDataUntil(event, kEnd);
      if (r == 0) {
        httpev->transport.parseReadData(httpev->rbuf.get());
        size_t n = httpev->transport.getContentLength();
        if (n > Protocol::BODYLEN_LIMIT) {
          RDDLOG(WARN) << "big request, bodyLength=" << n;
        } else {
          RDDLOG(V3) << "bodyLength=" << n;
        }
        if (n == 0) {
          return 0;
        }
        httpev->rlen += n;
        switch (httpev->transport.state()) {
          case HTTPTransport::kOnReading: return 1;
          case HTTPTransport::kOnReadingFinish: return 0;
          case HTTPTransport::kError: return -1;
          default:
            throw std::runtime_error(
                to<std::string>("wrong http state: ", state));
        }
      }
      return r;
    }
    if (state == HTTPTransport::kOnReading) {
      int r = Protocol::readData(event);
      if (r == 0) {
        httpev->transport.parseReadData(httpev->rbuf.get());
      }
      return r;
    }
    return -1;
  }
};

} // namespace rdd
