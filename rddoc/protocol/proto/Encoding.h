/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>
#include <thrift/transport/TBufferTransports.h>
#include "rddoc/net/Protocol.h"
#include "rddoc/io/Cursor.h"
#include "rddoc/io/TypedIOBuf.h"
#include "rddoc/io/ZlibStreamCompressor.h"
#include "rddoc/io/ZlibStreamDecompressor.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/Memory.h"
#include "rddoc/util/String.h"

namespace rdd {
namespace proto {

// buf -> ibuf
inline bool decodeData(IOBuf* buf,
                       apache::thrift::transport::TMemoryBuffer* ibuf) {
  auto range = buf->coalesce();
  RDDLOG_IF(V4) {
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "decode thrift data: " << hex;
  }
  uint32_t header = *TypedIOBuf<uint32_t>(buf).data();
  RDDLOG(V3) << "decode thrift size: " << ntohl(header);
  range.advance(sizeof(uint32_t));
  ibuf->resetBuffer((uint8_t*)range.data(), range.size());
  return true;
}

// obuf -> buf
inline bool encodeData(IOBuf* buf,
                       apache::thrift::transport::TMemoryBuffer* obuf) {
  uint8_t* p;
  uint32_t n;
  obuf->getBuffer(&p, &n);
  RDDLOG(V3) << "encode thrift size: " << n;
  TypedIOBuf<uint32_t>(buf).push(htonl(n));
  rdd::io::Appender appender(buf, Protocol::CHUNK_SIZE);
  appender.pushAtMost(p, n);
  RDDLOG_IF(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "encode thrift data: " << hex;
  }
  return true;
}

}
}

