/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/event/Event.h"
#include "raster/protocol/http/HTTP1xCodec.h"
#include "raster/protocol/http/Transport.h"

namespace rdd {

class HTTPEvent : public Event,
                  public HTTP1xCodec::Callback,
                  public HTTPTransport {
public:
  enum TransportState {
    kInit,
    kOnReading,
    kOnReadingFinish,
    kOnWriting,
    kOnWritingFinish,
    kError,
  };

  HTTPEvent(const std::shared_ptr<Channel>& channel,
            const std::shared_ptr<Socket>& socket);

  virtual ~HTTPEvent() {}

  virtual void reset();

  // HTTP1xCodec::Callback
  virtual void onMessageBegin(HTTPMessage* msg);
  virtual void onHeadersComplete(std::unique_ptr<HTTPMessage> msg);
  virtual void onBody(std::unique_ptr<IOBuf> chain);
  virtual void onChunkHeader(size_t length);
  virtual void onChunkComplete();
  virtual void onTrailersComplete(std::unique_ptr<HTTPHeaders> trailers);
  virtual void onMessageComplete();
  virtual void onError(const HTTPException& error);

  // HTTPTransport
  virtual void sendHeaders(const HTTPMessage& headers, HTTPHeaderSize* size);
  virtual size_t sendBody(std::unique_ptr<IOBuf> body, bool includeEOM);
  virtual size_t sendChunkHeader(size_t length);
  virtual size_t sendChunkTerminator();
  virtual size_t sendTrailers(const HTTPHeaders& trailers);
  virtual size_t sendEOM();
  virtual size_t sendAbort();

  void parseReadData();
  void pushWriteData();

  TransportState state() const { return state_; }
  void setState(TransportState state) { state_ = state; }

  HTTPMessage* message() const { return msg_.get(); }
  IOBuf* body() const { return body_.get(); }
  HTTPHeaders* trailers() const { return trailers_.get(); }

private:
  TransportState state_;
  HTTP1xCodec codec_;
  std::unique_ptr<HTTPMessage> msg_;
  std::unique_ptr<IOBuf> body_;
  std::unique_ptr<HTTPHeaders> trailers_;

  IOBufQueue writeBuf_{IOBufQueue::cacheChainLength()};
};

} // namespace rdd
