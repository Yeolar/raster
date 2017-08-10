/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <zlib.h>
#include "raster/io/ZlibStreamDecompressor.h"

namespace rdd {

extern int64_t FLAGS_zlib_buffer_growth;
extern int64_t FLAGS_zlib_buffer_minsize;

class ZlibStreamCompressor {
 public:
  explicit ZlibStreamCompressor(ZlibCompressionType type, int level);

  ~ZlibStreamCompressor();

  void init(ZlibCompressionType type, int level);

  std::unique_ptr<rdd::IOBuf> compress(const rdd::IOBuf* in,
                                       bool trailer = true);

  int getStatus() { return status_; }

  bool hasError() { return status_ != Z_OK && status_ != Z_STREAM_END; }

  bool finished() { return status_ == Z_STREAM_END; }

 private:
  ZlibCompressionType type_{ZlibCompressionType::NONE};
  int level_{Z_DEFAULT_COMPRESSION};
  z_stream zlibStream_;
  int status_{-1};
};

} // namespace rdd
