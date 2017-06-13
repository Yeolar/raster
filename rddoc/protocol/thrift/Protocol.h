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

namespace thrift {

inline void setSeqId(char* buf, int32_t seqid) {
  char* p = buf;
  int32_t n = *((int32_t*)p)++;
  n = n < 0 ? *((int32_t*)p)++ : n + 1;
  *(int32_t*)(p + n) = seqid;
}

inline int32_t getSeqId(char* buf) {
  char* p = buf;
  int32_t n = *((int32_t*)p)++;
  n = n < 0 ? *((int32_t*)p)++ : n + 1;
  return *(int32_t*)(p + n);
}

}

}
