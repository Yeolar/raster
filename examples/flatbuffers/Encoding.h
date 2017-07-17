/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <arpa/inet.h>
#include <flatbuffers/flatbuffers.h>
#include "rddoc/net/Protocol.h"
#include "rddoc/io/Cursor.h"
#include "rddoc/io/TypedIOBuf.h"

namespace rdd {
namespace binary {

// obuf -> buf
inline bool encodeData(IOBuf* buf, ::flatbuffers::FlatBufferBuilder* obuf) {
  uint8_t* p = obuf->GetBufferPointer();
  uint32_t n = obuf->GetSize();
  RDDLOG(V3) << "encode binary size: " << n;
  TypedIOBuf<uint32_t>(buf).push(htonl(n));
  rdd::io::Appender appender(buf, Protocol::CHUNK_SIZE);
  appender.pushAtMost(p, n);
  RDDLOG_ON(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "encode binary data: " << hex;
  }
  return true;
}

}
}
