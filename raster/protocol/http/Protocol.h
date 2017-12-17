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
  enum {
    ON_HEADERS,
    ON_BODY,
  };

  HTTPProtocol() : Protocol() {}
  virtual ~HTTPProtocol() {}

  virtual int readData(Event* event) {
    static const ByteRange kEnd((uint8_t*)"\r\n\r\n", 4);
    HTTPEvent* httpev = reinterpret_cast<HTTPEvent*>(event);

    if (httpev->state() == HTTPEvent::ON_HEADERS) {
      int r = Protocol::readDataUntil(event, kEnd);
      if (r == 0) {
        httpev->onHeaders();
        switch (httpev->state()) {
          case HTTPEvent::ON_BODY: return 1;
          case HTTPEvent::FINISH: return 0;
          case HTTPEvent::ERROR: return -1;
          case HTTPEvent::INIT:
          case HTTPEvent::ON_HEADERS:
          default:
            throw std::runtime_error(
                to<std::string>("meet wrong http state: ", httpev->state()));
        }
      }
      return r;
    } else {
      int r = Protocol::readData(event);
      if (r == 0) {
        httpev->onBody();
      }
      return r;
    }
  }
};

} // namespace rdd
