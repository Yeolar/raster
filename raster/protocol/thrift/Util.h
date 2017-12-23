/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/util/Logging.h"

namespace rdd {
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
