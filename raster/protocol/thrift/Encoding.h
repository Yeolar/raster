/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>
#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/net/Protocol.h"
#include "raster/io/Cursor.h"
#include "raster/io/TypedIOBuf.h"
#include "raster/io/ZlibStreamCompressor.h"
#include "raster/io/ZlibStreamDecompressor.h"
#include "raster/util/Logging.h"
#include "raster/util/Memory.h"
#include "raster/util/String.h"

namespace rdd {
namespace thrift {

// buf -> ibuf
inline bool decodeData(std::unique_ptr<IOBuf>& buf,
                       apache::thrift::transport::TMemoryBuffer* ibuf) {
  auto range = buf->coalesce();
  RDDLOG_ON(V4) {
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "decode thrift data: " << hex;
  }
  uint32_t header = *TypedIOBuf<uint32_t>(buf.get()).data();
  RDDLOG(V3) << "decode thrift size: " << ntohl(header);
  range.advance(sizeof(uint32_t));
  ibuf->resetBuffer((uint8_t*)range.data(), range.size());
  return true;
}

inline bool decodeZlibData(std::unique_ptr<IOBuf>& buf,
                           apache::thrift::transport::TMemoryBuffer* ibuf) {
  RDDLOG_ON(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "decode thrift data: " << hex;
  }
  RDDLOG(V3) << "decode thrift size: " << buf->computeChainDataLength();
  auto codec = make_unique<ZlibStreamDecompressor>(
      ZlibCompressionType::DEFLATE);
  auto decompressed = codec->decompress(buf.get());
  if (!decompressed) {
    return false;
  }
  auto data = decompressed.get()->coalesce();
  ibuf->resetBuffer((uint8_t*)data.data(), data.size(),
                    apache::thrift::transport::TMemoryBuffer::COPY);
  return true;
}

// obuf -> buf
inline bool encodeData(std::unique_ptr<IOBuf>& buf,
                       apache::thrift::transport::TMemoryBuffer* obuf) {
  uint8_t* p;
  uint32_t n;
  obuf->getBuffer(&p, &n);
  RDDLOG(V3) << "encode thrift size: " << n;
  TypedIOBuf<uint32_t>(buf.get()).push(htonl(n));
  rdd::io::Appender appender(buf.get(), Protocol::CHUNK_SIZE);
  appender.pushAtMost(p, n);
  RDDLOG_ON(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "encode thrift data: " << hex;
  }
  return true;
}

inline bool encodeZlibData(std::unique_ptr<IOBuf>& buf,
                           apache::thrift::transport::TMemoryBuffer* obuf) {
  uint8_t* p;
  uint32_t n;
  obuf->getBuffer(&p, &n);
  auto codec = make_unique<ZlibStreamCompressor>(
      ZlibCompressionType::DEFLATE, 9);
  auto original = IOBuf::wrapBuffer(p, n);
  auto compressed = codec->compress(original.get(), false);
  if (!compressed) {
    return false;
  }
  compressed->cloneInto(*buf);
  RDDLOG(V3) << "encode thrift size: " << buf->computeChainDataLength();
  RDDLOG_ON(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "encode thrift data: " << hex;
  }
  return true;
}

} // namespace thrift
} // namespace rdd
