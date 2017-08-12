/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Protocol.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"

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

namespace thrift {

inline void setSeqId(::apache::thrift::transport::TMemoryBuffer* buf,
                     int32_t seqid) {
  uint8_t* b;
  uint32_t n;
  buf->getBuffer(&b, &n);
  int32_t* p = (int32_t*)b;
  int32_t i = ntohl(*p++);
  i = i < 0 ? ntohl(*p++) : i + 1;
  if (n >= i + sizeof(int32_t)) {
    *(int32_t*)((uint8_t*)p + i) = htonl(seqid);
  } else {
    RDDLOG(WARN) << "invalid buf to set seqid";
  }
}

inline int32_t getSeqId(::apache::thrift::transport::TMemoryBuffer* buf) {
  uint8_t* b;
  uint32_t n;
  buf->getBuffer(&b, &n);
  int32_t* p = (int32_t*)b;
  int32_t i = ntohl(*p++);
  i = i < 0 ? ntohl(*p++) : i + 1;
  if (n >= i + sizeof(int32_t)) {
    return ntohl(*(int32_t*)((uint8_t*)p + i));
  } else {
    RDDLOG(WARN) << "invalid buf to get seqid";
    return 0;
  }
}

} // namespace thrift

} // namespace rdd
