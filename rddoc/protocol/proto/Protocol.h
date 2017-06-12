/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Protocol.h"

namespace rdd {

// protocol of thrift binary
class TBinaryProtocol : public HBinaryProtocol<uint32_t> {
public:
  TBinaryProtocol() : HBinaryProtocol<uint32_t>() {}
  virtual ~TBinaryProtocol() {}

  virtual size_t bodyLength(const uint32_t& header) const {
    return ntohl(header);
  }
};

// protocol of thrift framed
typedef TBinaryProtocol TFramedProtocol;

// protocol of thrift zlib
class TZlibProtocol : public ZlibBinaryProtocol {
public:
  TZlibProtocol() : ZlibBinaryProtocol() {}
  virtual ~TZlibProtocol() {}
};

}
