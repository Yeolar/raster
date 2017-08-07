/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Protocol.h"

namespace rdd {

// protocol of binary
class BinaryProtocol : public HBinaryProtocol<uint32_t> {
public:
  BinaryProtocol() : HBinaryProtocol<uint32_t>() {}
  virtual ~BinaryProtocol() {}

  virtual size_t bodyLength(const uint32_t& header) const {
    return ntohl(header);
  }
};

} // namespace rdd
