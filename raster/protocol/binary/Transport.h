/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/ZlibStreamCompressor.h"
#include "raster/io/ZlibStreamDecompressor.h"
#include "raster/net/Transport.h"
#include "raster/util/Memory.h"

namespace rdd {

class BinaryTransport : public Transport {
public:
  BinaryTransport() { reset(); }
  virtual ~BinaryTransport() {}

  virtual void reset();

  virtual void processReadData();

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
  virtual ~BinaryTransportFactory() {}

  virtual std::unique_ptr<Transport> create() {
    return make_unique<BinaryTransport>();
  }
};

class ZlibTransport : public Transport {
public:
  ZlibTransport() { reset(); }
  virtual ~ZlibTransport() {}

  virtual void reset();

  virtual void processReadData();

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
  virtual ~ZlibTransportFactory() {}

  virtual std::unique_ptr<Transport> create() {
    return make_unique<ZlibTransport>();
  }
};

} // namespace rdd
