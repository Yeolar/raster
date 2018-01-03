/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/compression/ZlibStreamCompressor.h"
#include "raster/io/compression/ZlibStreamDecompressor.h"
#include "raster/net/Transport.h"
#include "raster/util/Memory.h"

namespace rdd {

class BinaryTransport : public Transport {
 public:
  BinaryTransport() { reset(); }
  ~BinaryTransport() override {}

  void reset() override;

  void processReadData() override;

  size_t onIngress(const IOBuf& buf);

  void sendHeader(uint32_t header);
  size_t sendBody(std::unique_ptr<IOBuf> body);

  uint32_t header;
  std::unique_ptr<IOBuf> body;

 private:
  uint8_t headerBuf_[4];
  size_t headerSize_;
  bool headersComplete_;
};

class BinaryTransportFactory : public TransportFactory {
 public:
  BinaryTransportFactory() {}
  ~BinaryTransportFactory() override {}

  std::unique_ptr<Transport> create() override {
    return make_unique<BinaryTransport>();
  }
};

class ZlibTransport : public Transport {
 public:
  ZlibTransport() { reset(); }
  ~ZlibTransport() override {}

  void reset() override;

  void processReadData() override;

  size_t onIngress(const IOBuf& buf);

  size_t sendBody(std::unique_ptr<IOBuf> body);

  std::unique_ptr<IOBuf> body;

 private:
  std::unique_ptr<ZlibStreamCompressor> compressor_;
  std::unique_ptr<ZlibStreamDecompressor> decompressor_;
};

class ZlibTransportFactory : public TransportFactory {
 public:
  ZlibTransportFactory() {}
  ~ZlibTransportFactory() override {}

  std::unique_ptr<Transport> create() override {
    return make_unique<ZlibTransport>();
  }
};

} // namespace rdd
