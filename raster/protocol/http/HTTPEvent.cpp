/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/Protocol.h"
#include "raster/protocol/http/HTTPEvent.h"
#include "raster/protocol/http/Util.h"

namespace rdd {

HTTPEvent::HTTPEvent(const std::shared_ptr<Channel>& channel,
                     const std::shared_ptr<Socket>& socket)
  : Event(channel, socket) {
  state_ = INIT;
}

void HTTPEvent::reset() {
  Event::reset();
  state_ = ON_HEADERS;
}

void HTTPEvent::onHeaders() {
  IOBuf* buf = rbuf().get();
  headerSize_ = buf->computeChainDataLength();

  StringPiece data(buf->coalesce());
  auto eol = data.find("\r\n");
  StringPiece method, uri, version;

  if (!split(' ', data.subpiece(0, eol), method, uri, version)) {
    RDDLOG(INFO) << *this << " malformed HTTP request line";
    state_ = ERROR;
    return;
  }
  if (!version.startsWith("HTTP/")) {
    RDDLOG(INFO) << *this << " malformed HTTP version in HTTP request";
    state_ = ERROR;
    return;
  }

  headers_ = std::make_shared<HTTPHeaders>(data.subpiece(eol + 2));
  request_ = std::make_shared<HTTPRequest>(method, uri, version, headers_,
                                           true,  // support xheaders
                                           peer().host);
  auto clen = headers_->get("Content-Length");

  if (clen.empty()) {
    state_ = FINISH;
  } else {
    size_t n = to<size_t>(clen);
    if (n > Protocol::BODYLEN_LIMIT) {
      RDDLOG(WARN) << *this << " big request, bodyLength=" << n;
    } else {
      RDDLOG(V3) << *this << " bodyLength=" << n;
    }
    /* TODO
    if (headers_->get("Expect") == "100-continue") {
      wbuf()->append("HTTP/1.1 100 (Continue)\r\n\r\n");
    }*/
    rlen() += n;
    state_ = ON_BODY;
  }
}

void HTTPEvent::onBody() {
  IOBuf* buf = rbuf().get();
  StringPiece data(buf->coalesce());
  request_->body = data.subpiece(headerSize_);

  if (request_->method == "POST" ||
      request_->method == "PATCH" ||
      request_->method == "PUT") {
    parseBodyArguments(headers_->get("Content-Type"),
                       data, request_->arguments, request_->files);
  }
  state_ = FINISH;
}

} // namespace rdd
