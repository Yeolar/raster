/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <zlib.h>
#include <gflags/gflags.h>

#include "raster/io/IOBuf.h"

DECLARE_int64(zlib_decompresser_buffer_growth);
DECLARE_int64(zlib_decompresser_buffer_minsize);

namespace rdd {

enum class ZlibCompressionType: int {
  NONE = 0,
  DEFLATE = 15,
  GZIP = 31
};

class ZlibStreamDecompressor {
 public:
  explicit ZlibStreamDecompressor(ZlibCompressionType type);

  ZlibStreamDecompressor() { }

  ~ZlibStreamDecompressor();

  void init(ZlibCompressionType type);

  std::unique_ptr<rdd::IOBuf> decompress(const rdd::IOBuf* in);

  int getStatus() { return status_; }

  bool hasError() { return status_ != Z_OK && status_ != Z_STREAM_END; }

  bool finished() { return status_ == Z_STREAM_END; }

 private:
  ZlibCompressionType type_{ZlibCompressionType::NONE};
  z_stream zlibStream_;
  int status_{-1};
};

} // namespace rdd
