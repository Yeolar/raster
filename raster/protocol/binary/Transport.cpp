/*
 * Copyright (C) 2017, Yeolar
 */

#include <arpa/inet.h>

#include "raster/protocol/binary/Transport.h"

namespace rdd {

void BinaryTransport::reset() {
  state_ = kInit;
  headerSize_ = 0;
  headersComplete_ = false;
  header = 0;
  body->clear();
}

void BinaryTransport::processReadData() {
  const IOBuf* buf;
  while ((buf = readBuf_.front()) != nullptr && buf->length() != 0) {
    size_t bytesParsed = onIngress(*buf);
    if (bytesParsed == 0) {
      break;
    }
    readBuf_.trimStart(bytesParsed);
  }
}

size_t BinaryTransport::onIngress(const IOBuf& buf) {
  std::unique_ptr<IOBuf> clone(buf.clone());
  if (!headersComplete_) {
    size_t headerLeft = sizeof(header) - headerSize_;
    size_t headerCopy = std::min(headerLeft, buf.length());
    memcpy(headerBuf_ + headerSize_, buf.data(), headerCopy);
    headerSize_ += headerCopy;
    clone->trimStart(headerCopy);
    if (headerSize_ == sizeof(header)) {
      header = ntohl(*(uint32_t*)headerBuf_);
      headersComplete_ = true;
    }
  }
  if (clone->length() > 0) {
    if (body) {
      body->appendChain(std::move(clone));
    } else {
      body = std::move(clone);
    }
  }
  if (body && body->computeChainDataLength() == header) {
    state_ = kFinish;
  }
  return buf.length();
}

void BinaryTransport::sendHeader(uint32_t header) {
  uint32_t n = htonl(header);
  writeBuf_.append(&n, sizeof(n));
}

size_t BinaryTransport::sendBody(std::unique_ptr<IOBuf> body) {
  size_t n = body->computeChainDataLength();
  writeBuf_.append(std::move(body));
  return n;
}

void ZlibTransport::reset() {
  compressor_.reset(new ZlibStreamCompressor(ZlibCompressionType::DEFLATE, 9));
  decompressor_.reset(new ZlibStreamDecompressor(ZlibCompressionType::DEFLATE));
  body->clear();
}

void ZlibTransport::processReadData() {
  const IOBuf* buf;
  while ((buf = readBuf_.front()) != nullptr && buf->length() != 0) {
    size_t bytesParsed = onIngress(*buf);
    if (bytesParsed == 0) {
      break;
    }
    readBuf_.trimStart(bytesParsed);
  }
}

size_t ZlibTransport::onIngress(const IOBuf& buf) {
  auto decompressed = decompressor_->decompress(&buf);
  if (decompressed->length() > 0) {
    if (body) {
      body->appendChain(std::move(decompressed));
    } else {
      body = std::move(decompressed);
    }
  }
  if (decompressor_->finished()) {
    state_ = kFinish;
  } else if (decompressor_->hasError()) {
    state_ = kError;
  }
  return decompressed->length();
}

size_t ZlibTransport::sendBody(std::unique_ptr<IOBuf> body) {
  size_t n = body->computeChainDataLength();
  auto compressed = compressor_->compress(body.get(), false);
  writeBuf_.append(std::move(compressed));
  if (decompressor_->hasError()) {
    state_ = kError;
  }
  return n;
}

} // namespace rdd
