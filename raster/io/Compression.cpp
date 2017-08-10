/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Compression.h"
#include "raster/util/Conv.h"
#include "raster/util/Logging.h"
#include "raster/util/Memory.h"
#include "raster/util/ScopeGuard.h"

#include <zlib.h>

namespace rdd { namespace io {

Codec::Codec(CodecType type) : type_(type) { }

// Ensure consistent behavior in the nullptr case
std::unique_ptr<IOBuf> Codec::compress(const IOBuf* data) {
  uint64_t len = data->computeChainDataLength();
  if (len == 0) {
    return IOBuf::create(0);
  } else if (len > maxUncompressedLength()) {
    throw std::runtime_error("Codec: uncompressed length too large");
  }

  return doCompress(data);
}

std::unique_ptr<IOBuf> Codec::uncompress(const IOBuf* data,
                                         uint64_t uncompressedLength) {
  if (uncompressedLength == UNKNOWN_UNCOMPRESSED_LENGTH) {
    if (needsUncompressedLength()) {
      throw std::invalid_argument("Codec: uncompressed length required");
    }
  } else if (uncompressedLength > maxUncompressedLength()) {
    throw std::runtime_error("Codec: uncompressed length too large");
  }

  if (data->empty()) {
    if (uncompressedLength != UNKNOWN_UNCOMPRESSED_LENGTH &&
        uncompressedLength != 0) {
      throw std::runtime_error("Codec: invalid uncompressed length");
    }
    return IOBuf::create(0);
  }

  return doUncompress(data, uncompressedLength);
}

bool Codec::needsUncompressedLength() const {
  return doNeedsUncompressedLength();
}

uint64_t Codec::maxUncompressedLength() const {
  return doMaxUncompressedLength();
}

bool Codec::doNeedsUncompressedLength() const {
  return false;
}

uint64_t Codec::doMaxUncompressedLength() const {
  return UNLIMITED_UNCOMPRESSED_LENGTH;
}

namespace {

/**
 * No compression
 */
class NoCompressionCodec : public Codec {
 public:
  static std::unique_ptr<Codec> create(int level, CodecType type);
  explicit NoCompressionCodec(int level, CodecType type);

 private:
  std::unique_ptr<IOBuf> doCompress(const IOBuf* data);
  std::unique_ptr<IOBuf> doUncompress(
      const IOBuf* data,
      uint64_t uncompressedLength);
};

std::unique_ptr<Codec> NoCompressionCodec::create(int level, CodecType type) {
  return make_unique<NoCompressionCodec>(level, type);
}

NoCompressionCodec::NoCompressionCodec(int level, CodecType type)
  : Codec(type) {
  DCHECK(type == CodecType::NO_COMPRESSION);
  switch (level) {
  case COMPRESSION_LEVEL_DEFAULT:
  case COMPRESSION_LEVEL_FASTEST:
  case COMPRESSION_LEVEL_BEST:
    level = 0;
  }
  if (level != 0) {
    throw std::invalid_argument(to<std::string>(
        "NoCompressionCodec: invalid level ", level));
  }
}

std::unique_ptr<IOBuf> NoCompressionCodec::doCompress(
    const IOBuf* data) {
  return data->clone();
}

std::unique_ptr<IOBuf> NoCompressionCodec::doUncompress(
    const IOBuf* data,
    uint64_t uncompressedLength) {
  if (uncompressedLength != UNKNOWN_UNCOMPRESSED_LENGTH &&
      data->computeChainDataLength() != uncompressedLength) {
    throw std::runtime_error(to<std::string>(
        "NoCompressionCodec: invalid uncompressed length"));
  }
  return data->clone();
}

/**
 * Zlib codec
 */
class ZlibCodec : public Codec {
 public:
  static std::unique_ptr<Codec> create(int level, CodecType type);
  explicit ZlibCodec(int level, CodecType type);

 private:
  std::unique_ptr<IOBuf> doCompress(const IOBuf* data);
  std::unique_ptr<IOBuf> doUncompress(
      const IOBuf* data,
      uint64_t uncompressedLength);

  std::unique_ptr<IOBuf> addOutputBuffer(z_stream* stream, uint32_t length);
  bool doInflate(z_stream* stream, IOBuf* head, uint32_t bufferLength);

  int level_;
};

std::unique_ptr<Codec> ZlibCodec::create(int level, CodecType type) {
  return make_unique<ZlibCodec>(level, type);
}

ZlibCodec::ZlibCodec(int level, CodecType type) : Codec(type) {
  DCHECK(type == CodecType::ZLIB);
  switch (level) {
  case COMPRESSION_LEVEL_FASTEST:
    level = 1;
    break;
  case COMPRESSION_LEVEL_DEFAULT:
    level = Z_DEFAULT_COMPRESSION;
    break;
  case COMPRESSION_LEVEL_BEST:
    level = 9;
    break;
  }
  if (level != Z_DEFAULT_COMPRESSION && (level < 0 || level > 9)) {
    throw std::invalid_argument(to<std::string>(
        "ZlibCodec: invalid level: ", level));
  }
  level_ = level;
}

std::unique_ptr<IOBuf> ZlibCodec::addOutputBuffer(z_stream* stream,
                                                  uint32_t length) {
  RDDCHECK_EQ(stream->avail_out, 0);

  auto buf = IOBuf::create(length);
  buf->append(length);

  stream->next_out = buf->writableData();
  stream->avail_out = buf->length();

  return buf;
}

bool ZlibCodec::doInflate(z_stream* stream,
                          IOBuf* head,
                          uint32_t bufferLength) {
  if (stream->avail_out == 0) {
    head->prependChain(addOutputBuffer(stream, bufferLength));
  }

  int rc = inflate(stream, Z_NO_FLUSH);

  switch (rc) {
  case Z_OK:
    break;
  case Z_STREAM_END:
    return true;
  case Z_BUF_ERROR:
  case Z_NEED_DICT:
  case Z_DATA_ERROR:
  case Z_MEM_ERROR:
    throw std::runtime_error(to<std::string>(
        "ZlibCodec: inflate error: ", rc, ": ", stream->msg));
  default:
    RDDCHECK(false) << rc << ": " << stream->msg;
  }

  return false;
}

std::unique_ptr<IOBuf> ZlibCodec::doCompress(const IOBuf* data) {
  z_stream stream;
  stream.zalloc = nullptr;
  stream.zfree = nullptr;
  stream.opaque = nullptr;

  int rc = deflateInit(&stream, level_);
  if (rc != Z_OK) {
    throw std::runtime_error(to<std::string>(
        "ZlibCodec: deflateInit error: ", rc, ": ", stream.msg));
  }

  stream.next_in = stream.next_out = nullptr;
  stream.avail_in = stream.avail_out = 0;
  stream.total_in = stream.total_out = 0;

  bool success = false;

  SCOPE_EXIT {
    int rc = deflateEnd(&stream);
    // If we're here because of an exception, it's okay if some data
    // got dropped.
    RDDCHECK(rc == Z_OK || (!success && rc == Z_DATA_ERROR))
      << rc << ": " << stream.msg;
  };

  uint64_t uncompressedLength = data->computeChainDataLength();
  uint64_t maxCompressedLength = deflateBound(&stream, uncompressedLength);

  // Max 64MiB in one go
  constexpr uint32_t maxSingleStepLength = uint32_t(64) << 20;    // 64MiB
  constexpr uint32_t defaultBufferLength = uint32_t(4) << 20;     // 4MiB

  auto out = addOutputBuffer(
      &stream,
      (maxCompressedLength <= maxSingleStepLength ?
       maxCompressedLength :
       defaultBufferLength));

  for (auto& range : *data) {
    uint64_t remaining = range.size();
    uint64_t written = 0;
    while (remaining) {
      uint32_t step = (remaining > maxSingleStepLength ?
                       maxSingleStepLength : remaining);
      stream.next_in = const_cast<uint8_t*>(range.data() + written);
      stream.avail_in = step;
      remaining -= step;
      written += step;

      while (stream.avail_in != 0) {
        if (stream.avail_out == 0) {
          out->prependChain(addOutputBuffer(&stream, defaultBufferLength));
        }

        rc = deflate(&stream, Z_NO_FLUSH);

        RDDCHECK_EQ(rc, Z_OK) << stream.msg;
      }
    }
  }

  do {
    if (stream.avail_out == 0) {
      out->prependChain(addOutputBuffer(&stream, defaultBufferLength));
    }

    rc = deflate(&stream, Z_FINISH);
  } while (rc == Z_OK);

  RDDCHECK_EQ(rc, Z_STREAM_END) << stream.msg;

  out->prev()->trimEnd(stream.avail_out);

  success = true;  // we survived

  return out;
}

std::unique_ptr<IOBuf> ZlibCodec::doUncompress(const IOBuf* data,
                                               uint64_t uncompressedLength) {
  z_stream stream;
  stream.zalloc = nullptr;
  stream.zfree = nullptr;
  stream.opaque = nullptr;

  int rc = inflateInit(&stream);
  if (rc != Z_OK) {
    throw std::runtime_error(to<std::string>(
        "ZlibCodec: inflateInit error: ", rc, ": ", stream.msg));
  }

  stream.next_in = stream.next_out = nullptr;
  stream.avail_in = stream.avail_out = 0;
  stream.total_in = stream.total_out = 0;

  bool success = false;

  SCOPE_EXIT {
    int rc = inflateEnd(&stream);
    // If we're here because of an exception, it's okay if some data
    // got dropped.
    RDDCHECK(rc == Z_OK || (!success && rc == Z_DATA_ERROR))
      << rc << ": " << stream.msg;
  };

  // Max 64MiB in one go
  constexpr uint32_t maxSingleStepLength = uint32_t(64) << 20;    // 64MiB
  constexpr uint32_t defaultBufferLength = uint32_t(4) << 20;     // 4MiB

  auto out = addOutputBuffer(
      &stream,
      ((uncompressedLength != UNKNOWN_UNCOMPRESSED_LENGTH &&
        uncompressedLength <= maxSingleStepLength) ?
       uncompressedLength :
       defaultBufferLength));

  bool streamEnd = false;
  for (auto& range : *data) {
    if (range.empty()) {
      continue;
    }

    stream.next_in = const_cast<uint8_t*>(range.data());
    stream.avail_in = range.size();

    while (stream.avail_in != 0) {
      if (streamEnd) {
        throw std::runtime_error(to<std::string>(
            "ZlibCodec: junk after end of data"));
      }

      streamEnd = doInflate(&stream, out.get(), defaultBufferLength);
    }
  }

  while (!streamEnd) {
    streamEnd = doInflate(&stream, out.get(), defaultBufferLength);
  }

  out->prev()->trimEnd(stream.avail_out);

  if (uncompressedLength != UNKNOWN_UNCOMPRESSED_LENGTH &&
      uncompressedLength != stream.total_out) {
    throw std::runtime_error(to<std::string>(
        "ZlibCodec: invalid uncompressed length"));
  }

  success = true;  // we survived

  return out;
}

}  // namespace

std::unique_ptr<Codec> getCodec(CodecType type, int level) {
  typedef std::unique_ptr<Codec> (*CodecFactory)(int, CodecType);

  static CodecFactory codecFactories[
    static_cast<size_t>(CodecType::NUM_CODEC_TYPES)] = {
    nullptr,  // USER_DEFINED
    NoCompressionCodec::create,
    ZlibCodec::create,
  };

  size_t idx = static_cast<size_t>(type);
  if (idx >= static_cast<size_t>(CodecType::NUM_CODEC_TYPES)) {
    throw std::invalid_argument(to<std::string>(
        "Compression type ", idx, " not supported"));
  }
  auto factory = codecFactories[idx];
  if (!factory) {
    throw std::invalid_argument(to<std::string>(
        "Compression type ", idx, " not supported"));
  }
  auto codec = (*factory)(level, type);
  DCHECK_EQ(static_cast<size_t>(codec->type()), idx);
  return codec;
}

}}  // namespaces
