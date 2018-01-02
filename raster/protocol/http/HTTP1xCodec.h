/*
 * Copyright (c) 2015, Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <iostream>

#include "raster/3rd/http_parser/http_parser.h"
#include "raster/io/IOBufQueue.h"
#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPMessage.h"
#include "raster/util/Optional.h"

namespace rdd {

enum class TransportDirection : uint8_t {
  DOWNSTREAM,  // receives requests from client
  UPSTREAM     // sends requests to server
};

const char* getTransportDirectionString(TransportDirection dir);

TransportDirection operator!(TransportDirection dir);

std::ostream& operator<<(std::ostream& os, const TransportDirection dir);

/**
 * A parser&generator that can translate between an internal
 * representation of an HTTP request and HTTP/1.1.
 */
class HTTP1xCodec {
 public:
  /**
   * Callback interface that users of HTTP1xCodec must implement
   */
  class Callback {
   public:
    virtual void onMessageBegin(HTTPMessage* msg) = 0;

    virtual void onHeadersComplete(std::unique_ptr<HTTPMessage> msg) = 0;

    virtual void onBody(std::unique_ptr<IOBuf> chain) = 0;

    /*
     * onChunkHeader() will be called when the chunk header is received.  As
     * the chunk data arrives, it will be passed to the callback normally with
     * onBody() calls.  Note that the chunk data may arrive in multiple
     * onBody() calls: it is not guaranteed to arrive in a single onBody()
     * call.
     *
     * After the chunk data has been received and the terminating CRLF has been
     * received, onChunkComplete() will be called.
     */
    virtual void onChunkHeader(size_t length) {}

    virtual void onChunkComplete() {}

    virtual void onTrailersComplete(std::unique_ptr<HTTPHeaders> trailers) = 0;

    virtual void onMessageComplete() = 0;

    virtual void onError(const HTTPException& error) = 0;

    virtual ~Callback() {}
  };

  explicit HTTP1xCodec(TransportDirection direction,
                       bool forceUpstream1_1 = false);
  ~HTTP1xCodec() {}

  TransportDirection getTransportDirection() const {
    return transportDirection_;
  }

  /*
   * Parse ingress message.
   */

  void setCallback(Callback* callback) { callback_ = callback; }

  bool isBusy() const;

  void setParserPaused(bool paused);

  size_t onIngress(const IOBuf& buf);

  void onIngressEOF();

  bool isReusable() const;

  /*
   * Write parts for an egress message.
   */

  void generateHeader(IOBufQueue& writeBuf,
                      const HTTPMessage& msg,
                      bool eom = false,
                      HTTPHeaderSize* size = nullptr);

  /*
   * This will automatically generate a chunk header and footer around the data
   * if necessary (e.g. you haven't manually sent a chunk header and the
   * message should be chunked).
   */
  size_t generateBody(IOBufQueue& writeBuf,
                      std::unique_ptr<IOBuf> chain,
                      bool eom);

  size_t generateChunkHeader(IOBufQueue& writeBuf, size_t length);

  size_t generateChunkTerminator(IOBufQueue& writeBuf);

  size_t generateTrailers(IOBufQueue& writeBuf, const HTTPHeaders& trailers);

  size_t generateEOM(IOBufQueue& writeBuf);

  size_t generateAbort(IOBufQueue& writeBuf);

 private:
  enum class HeaderParseState : uint8_t {
    kParsingHeaderIdle,
    kParsingHeaderStart,
    kParsingHeaderName,
    kParsingHeaderValue,
    kParsingHeadersComplete,
    kParsingTrailerName,
    kParsingTrailerValue
  };

  enum class KeepaliveRequested : uint8_t {
    UNSET,
    ENABLED,  // incoming message requested keepalive
    DISABLED,   // incoming message disabled keepalive
  };

  void addDateHeader(IOBufQueue& writeBuf, size_t& len);

  bool isParsingHeaders() const {
    return (headerParseState_ > HeaderParseState::kParsingHeaderIdle) &&
       (headerParseState_ < HeaderParseState::kParsingHeadersComplete);
  }

  bool isParsingHeaderOrTrailerName() const {
    return (headerParseState_ == HeaderParseState::kParsingHeaderName) ||
        (headerParseState_ == HeaderParseState::kParsingTrailerName);
  }

  void onParserError(const char* what = nullptr);

  void pushHeaderNameAndValue(HTTPHeaders& hdrs);

  // Parser callbacks
  int onMessageBegin();
  int onURL(const char* buf, size_t len);
  int onReason(const char* buf, size_t len);
  int onHeaderField(const char* buf, size_t len);
  int onHeaderValue(const char* buf, size_t len);
  int onHeadersComplete(size_t len);
  int onBody(const char* buf, size_t len);
  int onChunkHeader(size_t len);
  int onChunkComplete();
  int onMessageComplete();

  Callback* callback_;
  http_parser parser_;
  const IOBuf* currentIngressBuf_;
  std::unique_ptr<HTTPMessage> msg_;
  std::unique_ptr<HTTPHeaders> trailers_;
  std::string currentHeaderName_;
  StringPiece currentHeaderNameStringPiece_;
  std::string currentHeaderValue_;
  std::string url_;
  std::string reason_;
  HTTPHeaderSize headerSize_;
  HeaderParseState headerParseState_;
  TransportDirection transportDirection_;
  KeepaliveRequested keepaliveRequested_; // only used in DOWNSTREAM mode
  bool forceUpstream1_1_:1; // Use HTTP/1.1 upstream even if msg is 1.0
  bool parserActive_:1;
  bool pendingEOF_:1;
  bool parserPaused_:1;
  bool parserError_:1;
  bool requestPending_:1;
  bool responsePending_:1;
  bool egressChunked_:1;
  bool inChunk_:1;
  bool lastChunkWritten_:1;
  bool keepalive_:1;
  bool disableKeepalivePending_:1;
  // TODO: replace the 2 booleans below with an enum "request method"
  bool connectRequest_:1;
  bool headRequest_:1;
  bool expectNoResponseBody_:1;
  bool mayChunkEgress_:1;
  bool is1xxResponse_:1;
  bool inRecvLastChunk_:1;
  bool headersComplete_:1;

  // C-callable wrappers for the http_parser callbacks
  static int onMessageBeginCB(http_parser* parser);
  static int onUrlCB(http_parser* parser, const char* buf, size_t len);
  static int onReasonCB(http_parser* parser, const char* buf, size_t len);
  static int onHeaderFieldCB(http_parser* parser, const char* buf, size_t len);
  static int onHeaderValueCB(http_parser* parser, const char* buf, size_t len);
  static int onHeadersCompleteCB(http_parser* parser,
                                 const char* buf, size_t len);
  static int onBodyCB(http_parser* parser, const char* buf, size_t len);
  static int onChunkHeaderCB(http_parser* parser);
  static int onChunkCompleteCB(http_parser* parser);
  static int onMessageCompleteCB(http_parser* parser);

  static void initParserSettings() __attribute__ ((__constructor__));

  static http_parser_settings kParserSettings;
};

} // namespace rdd
