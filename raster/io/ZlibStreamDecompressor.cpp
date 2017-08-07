/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/ZlibStreamDecompressor.h"
#include "raster/io/Cursor.h"

// IOBuf uses 24 bytes of data for bookeeping purposes, so requesting for 480
// bytes of data will be rounded up to an allocation of 512 bytes.  (If we
// requested for 512 bytes exactly IOBuf would round this up to 768, since it
// needs 24 extra bytes of its own.)

namespace rdd {

int64_t FLAGS_zlib_buffer_growth = 480;
int64_t FLAGS_zlib_buffer_minsize = 64;

void ZlibStreamDecompressor::init(ZlibCompressionType type) {
  DCHECK(type_ == ZlibCompressionType::NONE) << "Must be uninitialized";
  type_ = type;
  status_ = Z_OK;
  zlibStream_.zalloc = Z_NULL;
  zlibStream_.zfree = Z_NULL;
  zlibStream_.opaque = Z_NULL;
  zlibStream_.total_in = 0;
  zlibStream_.next_in = Z_NULL;
  zlibStream_.avail_in = 0;
  zlibStream_.avail_out = 0;
  zlibStream_.next_out = Z_NULL;

  DCHECK(type != ZlibCompressionType::NONE);
  status_ = inflateInit2(&zlibStream_, static_cast<int>(type_));
}

ZlibStreamDecompressor::ZlibStreamDecompressor(ZlibCompressionType type)
    : type_(ZlibCompressionType::NONE), status_(Z_OK) {
  init(type);
}

ZlibStreamDecompressor::~ZlibStreamDecompressor() {
  if (type_ != ZlibCompressionType::NONE) {
    status_ = inflateEnd(&zlibStream_);
  }
}

std::unique_ptr<IOBuf> ZlibStreamDecompressor::decompress(const IOBuf* in) {
  auto out = IOBuf::create(FLAGS_zlib_buffer_growth);
  auto appender = rdd::io::Appender(out.get(),
      FLAGS_zlib_buffer_growth);

  const IOBuf* crtBuf = in;
  size_t offset = 0;
  while (true) {
    // Advance to the next IOBuf if necessary
    DCHECK_GE(crtBuf->length(), offset);
    if (crtBuf->length() == offset) {
      crtBuf = crtBuf->next();
      offset = 0;
      if (crtBuf == in) {
        // We hit the end of the IOBuf chain, and are done.
        break;
      }
    }

    // The decompressor says we should have finished
    if (status_ == Z_STREAM_END) {
      // we convert this into a stream error
      status_ = Z_STREAM_ERROR;
      // we should probably bump up a counter here
      RDDLOG(ERROR) << "error uncompressing buffer: reached end of zlib data "
        "before the end of the buffer";
      return nullptr;
    }

    // Ensure there is space in the output IOBuf
    appender.ensure(FLAGS_zlib_buffer_minsize);
    DCHECK_GT(appender.length(), 0);

    const size_t origAvailIn = crtBuf->length() - offset;
    zlibStream_.next_in = const_cast<uint8_t*>(crtBuf->data() + offset);
    zlibStream_.avail_in = origAvailIn;
    zlibStream_.next_out = appender.writableData();
    zlibStream_.avail_out = appender.length();
    status_ = inflate(&zlibStream_, Z_PARTIAL_FLUSH);
    if (status_ != Z_OK && status_ != Z_STREAM_END) {
      RDDLOG(INFO) << "error uncompressing buffer: r=" << status_;
      return nullptr;
    }

    // Adjust the input offset ahead
    auto inConsumed = origAvailIn - zlibStream_.avail_in;
    offset += inConsumed;
    // Move output buffer ahead
    auto outMove = appender.length() - zlibStream_.avail_out;
    appender.append(outMove);
  }

  return out;
}

} // namespace rdd
