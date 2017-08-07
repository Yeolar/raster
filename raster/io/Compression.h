/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstdint>
#include <limits>
#include <memory>

#include "raster/io/IOBuf.h"

/**
 * Compression / decompression over IOBufs
 */

namespace rdd { namespace io {

enum class CodecType {
  /**
   * This codec type is not defined; getCodec() will throw an exception
   * if used. Useful if deriving your own classes from Codec without
   * going through the getCodec() interface.
   */
  USER_DEFINED = 0,

  /**
   * Use no compression.
   * Levels supported: 0
   */
  NO_COMPRESSION = 1,

  /**
   * Use zlib compression.
   * Levels supported: 0 = no compression, 1 = fast, ..., 9 = best; default = 6
   */
  ZLIB = 2,

  NUM_CODEC_TYPES = 3,
};

class Codec {
 public:
  virtual ~Codec() { }

  /**
   * Return the maximum length of data that may be compressed with this codec.
   * NO_COMPRESSION and ZLIB support arbitrary lengths.
   * May return UNLIMITED_UNCOMPRESSED_LENGTH if unlimited.
   */
  uint64_t maxUncompressedLength() const;

  /**
   * Return the codec's type.
   */
  CodecType type() const { return type_; }

  /**
   * Does this codec need the exact uncompressed length on decompression?
   */
  bool needsUncompressedLength() const;

  /**
   * Compress data, returning an IOBuf (which may share storage with data).
   * Throws std::invalid_argument if data is larger than
   * maxUncompressedLength().
   *
   * Regardless of the behavior of the underlying compressor, compressing
   * an empty IOBuf chain will return an empty IOBuf chain.
   */
  std::unique_ptr<IOBuf> compress(const rdd::IOBuf* data);

  /**
   * Uncompress data. Throws std::runtime_error on decompression error.
   *
   * Some codecs (LZ4) require the exact uncompressed length; this is indicated
   * by needsUncompressedLength().
   *
   * For other codes (zlib), knowing the exact uncompressed length ahead of
   * time might be faster.
   *
   * Regardless of the behavior of the underlying compressor, uncompressing
   * an empty IOBuf chain will return an empty IOBuf chain.
   */
  static constexpr uint64_t UNKNOWN_UNCOMPRESSED_LENGTH = uint64_t(-1);
  static constexpr uint64_t UNLIMITED_UNCOMPRESSED_LENGTH = uint64_t(-2);

  std::unique_ptr<IOBuf> uncompress(
      const IOBuf* data,
      uint64_t uncompressedLength = UNKNOWN_UNCOMPRESSED_LENGTH);

 protected:
  explicit Codec(CodecType type);

 private:
  // default: no limits (save for special value UNKNOWN_UNCOMPRESSED_LENGTH)
  virtual uint64_t doMaxUncompressedLength() const;
  // default: doesn't need uncompressed length
  virtual bool doNeedsUncompressedLength() const;
  virtual std::unique_ptr<IOBuf> doCompress(const rdd::IOBuf* data) = 0;
  virtual std::unique_ptr<IOBuf> doUncompress(const rdd::IOBuf* data,
                                              uint64_t uncompressedLength) = 0;

  CodecType type_;
};

constexpr int COMPRESSION_LEVEL_FASTEST = -1;
constexpr int COMPRESSION_LEVEL_DEFAULT = -2;
constexpr int COMPRESSION_LEVEL_BEST = -3;

/**
 * Return a codec for the given type. Throws on error.  The level
 * is a non-negative codec-dependent integer indicating the level of
 * compression desired, or one of the following constants:
 *
 * COMPRESSION_LEVEL_FASTEST is fastest (uses least CPU / memory,
 *   worst compression)
 * COMPRESSION_LEVEL_DEFAULT is the default (likely a tradeoff between
 *   FASTEST and BEST)
 * COMPRESSION_LEVEL_BEST is the best compression (uses most CPU / memory,
 *   best compression)
 *
 * When decompressing, the compression level is ignored. All codecs will
 * decompress all data compressed with the a codec of the same type, regardless
 * of compression level.
 */
std::unique_ptr<Codec> getCodec(CodecType type,
                                int level = COMPRESSION_LEVEL_DEFAULT);

}}  // namespaces

