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
    if (httpev->state() == HTTPEvent::INIT) {
      int r = Protocol::readDataUntil(event, kEnd);
      if (r == 0) {
        httpev->onReadingHeaders();
        switch (httpev->state()) {
          case HTTPEvent::ON_READING: return 1;
          case HTTPEvent::ON_READING_FINISH: return 0;
          case HTTPEvent::ERROR: return -1;
          default:
            throw std::runtime_error(
                to<std::string>("wrong http state: ", httpev->state()));
        }
      }
      return r;
    }
    if (httpev->state() == HTTPEvent::ON_READING) {
      int r = Protocol::readData(event);
      if (r == 0) {
        httpev->onReadingBody();
      }
      return r;
    }
    return -1;
  }

  virtual int writeData(Event* event) {
    HTTPEvent* httpev = reinterpret_cast<HTTPEvent*>(event);
    if (httpev->state() == HTTPEvent::ON_WRITING) {
      httpev->onWriting();
    }
    return Protocol::writeData(event);
  }
};

} // namespace rdd
