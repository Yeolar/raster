/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <arpa/inet.h>
#include "rddoc/net/Protocol.h"
#include "rddoc/io/Cursor.h"
#include "rddoc/io/TypedIOBuf.h"

namespace rdd {
namespace binary {

// buf -> ibuf
inline bool decodeData(IOBuf* buf, std::vector<uint8_t>* ibuf) {
  auto range = buf->coalesce();
  RDDLOG_ON(V4) {
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "decode binary data: " << hex;
  }
  uint32_t header = *TypedIOBuf<uint32_t>(buf).data();
  RDDLOG(V3) << "decode binary size: " << ntohl(header);
  range.advance(sizeof(uint32_t));
  ibuf->assign(range.begin(), range.end());
  return true;
}

// obuf -> buf
inline bool encodeData(IOBuf* buf, std::vector<uint8_t>* obuf) {
  uint8_t* p = (uint8_t*)&(*obuf)[0];
  uint32_t n = obuf->size();
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
