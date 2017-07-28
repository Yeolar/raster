/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Protocol.h"

namespace rdd {

// protocol of proto binary
class PBBinaryProtocol : public HBinaryProtocol<uint32_t> {
public:
  PBBinaryProtocol() : HBinaryProtocol<uint32_t>() {}
  virtual ~PBBinaryProtocol() {}

  virtual size_t bodyLength(const uint32_t& header) const {
    return ntohl(header);
  }
};

} // namespace rdd
