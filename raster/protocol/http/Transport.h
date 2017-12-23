/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Transport.h"
#include "raster/protocol/http/HTTP1xCodec.h"

namespace rdd {

class HTTPTransport : public Transport, public HTTP1xCodec::Callback {
public:
  HTTPTransport(TransportDirection direction)
    : codec_(direction) {
    codec_.setCallback(this);
  }

  virtual void reset();

  virtual void processReadData();

  // HTTP1xCodec::Callback
  virtual void onMessageBegin(HTTPMessage* msg);
  virtual void onHeadersComplete(std::unique_ptr<HTTPMessage> msg);
  virtual void onBody(std::unique_ptr<IOBuf> chain);
  virtual void onChunkHeader(size_t length);
  virtual void onChunkComplete();
  virtual void onTrailersComplete(std::unique_ptr<HTTPHeaders> trailers);
  virtual void onMessageComplete();
  virtual void onError(const HTTPException& error);

  void sendHeaders(const HTTPMessage& headers, HTTPHeaderSize* size);
  size_t sendBody(std::unique_ptr<IOBuf> body, bool includeEOM);
  size_t sendChunkHeader(size_t length);
  size_t sendChunkTerminator();
  size_t sendTrailers(const HTTPHeaders& trailers);
  size_t sendEOM();
  size_t sendAbort();

  std::unique_ptr<HTTPMessage> headers;
  std::unique_ptr<IOBuf> body;
  std::unique_ptr<HTTPHeaders> trailers;

private:
  HTTP1xCodec codec_;
};

class HTTPTransportFactory : public TransportFactory {
public:
  HTTPTransportFactory(TransportDirection direction)
    : direction_(direction) {}

  virtual ~HTTPTransportFactory() {}

  virtual std::unique_ptr<Transport> create() {
    return make_unique<HTTPTransport>(direction_);
  }

private:
  TransportDirection direction_;
};

} // namespace rdd
