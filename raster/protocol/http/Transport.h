/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/protocol/http/HTTP1xCodec.h"

namespace rdd {

class HTTPTransport : public HTTP1xCodec::Callback {
public:
  enum TransportState {
    kInit,
    kOnReading,
    kOnReadingFinish,
    kOnWriting,
    kOnWritingFinish,
    kError,
  };

  HTTPTransport(TransportDirection direction)
    : codec_(direction),
      state_(kInit) {
    codec_.setCallback(this);
  }

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

  void parseReadData(IOBuf* buf);
  void pushWriteData(IOBuf* buf);

  size_t getContentLength();

  TransportState state() const { return state_; }
  void setState(TransportState state) { state_ = state; }

  std::unique_ptr<HTTPMessage> headers;
  std::unique_ptr<IOBuf> body;
  std::unique_ptr<HTTPHeaders> trailers;

private:
  TransportState state_;
  HTTP1xCodec codec_;
  IOBufQueue writeBuf_{IOBufQueue::cacheChainLength()};
};

} // namespace rdd
