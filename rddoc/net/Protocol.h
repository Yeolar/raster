/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include "rddoc/net/Event.h"
#include "rddoc/io/IOBuf.h"
#include "rddoc/io/TypedIOBuf.h"
#include "rddoc/util/Logging.h"

namespace rdd {

class Protocol {
public:
  static constexpr uint32_t CHUNK_SIZE = 4096;
  static constexpr size_t BODYLEN_LIMIT = 104857600;  // 100MB

  Protocol() {}
  virtual ~Protocol() {}

  /*
   * return:
   *  -1: error
   *   0: complete
   *   1: again
   *   2: peer is closed
   */

  virtual int readData(Event* event);
  virtual int readDataUntil(Event* event, ByteRange bytes);

  virtual int writeData(Event* event);
};

// protocol of struct header and binary body
template <class H>
class HBinaryProtocol : public Protocol {
public:
  HBinaryProtocol() : Protocol() {}
  virtual ~HBinaryProtocol() {}

  virtual int readData(Event* event) {
    IOBuf* buf = event->rbuf();
    if (buf->empty()) {
      event->rlen() = headerSize();
    }
    int r = Protocol::readData(event);
    if (buf->computeChainDataLength() == headerSize() && r == 0) {
      const H* header = TypedIOBuf<H>(buf).data();
      size_t n = bodyLength(*header);
      if (n > BODYLEN_LIMIT) {
        RDD_EVLOG(WARN, event) << "big request, bodyLength=" << n;
      }
      event->rlen() = n;
      r = Protocol::readData(event);
    }
    return r;
  }

  size_t headerSize() const { return sizeof(H); }

  virtual size_t bodyLength(const H& header) const = 0;
};

class ZlibBinaryProtocol : public Protocol {
public:
  ZlibBinaryProtocol() : Protocol() {}
  virtual ~ZlibBinaryProtocol() {}

  virtual int readData(Event* event) {
    return Protocol::readDataUntil(
        event, ByteRange((uint8_t*)"\x00\x00\xff\xff", 4));
  }
};

}

