/*
 * Copyright (c) 2015, Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/protocol/http/HTTP1xCodec.h"

#include "accelerator/io/Cursor.h"
#include "raster/raster-config.h"
#include "raster/protocol/http/Util.h"

namespace {

const char CRLF[] = "\r\n";

unsigned u64toa(uint64_t value, void* dst) {
  // Write backwards.
  char* next = (char*)dst;
  char* start = next;
  do {
    *next++ = '0' + (value % 10);
    value /= 10;
  } while (value != 0);
  unsigned length = next - start;

  // Reverse in-place.
  next--;
  while (next > start) {
    char swap = *next;
    *next = *start;
    *start = swap;
    next--;
    start++;
  }
  return length;
}

void appendUint(acc::IOBufQueue& queue, size_t& len, uint64_t value) {
  char buf[32];
  size_t encodedLen = u64toa(value, buf);
  queue.append(buf, encodedLen);
  len += encodedLen;
}

#define appendLiteral(queue, len, str) (len) += (sizeof(str) - 1); \
  (queue).append(str, sizeof(str) - 1)

void appendString(acc::IOBufQueue& queue, size_t& len, const std::string& str) {
  queue.append(str.data(), str.length());
  len += str.length();
}

const std::pair<uint8_t, uint8_t> kHTTPVersion10(1, 0);

} // anonymous namespace

namespace rdd {

const char* getTransportDirectionString(TransportDirection dir) {
  switch (dir) {
    case TransportDirection::UPSTREAM: return "upstream";
    case TransportDirection::DOWNSTREAM: return "downstream";
  }
  return "";
}

TransportDirection operator!(TransportDirection dir) {
  return dir == TransportDirection::DOWNSTREAM ?
    TransportDirection::UPSTREAM : TransportDirection::DOWNSTREAM;
}

std::ostream& operator<<(std::ostream& os, const TransportDirection dir) {
  os << getTransportDirectionString(dir);
  return os;
}

http_parser_settings HTTP1xCodec::kParserSettings;

HTTP1xCodec::HTTP1xCodec(TransportDirection direction, bool forceUpstream1_1)
  : callback_(nullptr),
    currentIngressBuf_(nullptr),
    headerParseState_(HeaderParseState::kParsingHeaderIdle),
    transportDirection_(direction),
    keepaliveRequested_(KeepaliveRequested::UNSET),
    forceUpstream1_1_(forceUpstream1_1),
    parserActive_(false),
    pendingEOF_(false),
    parserPaused_(false),
    parserError_(false),
    requestPending_(false),
    responsePending_(false),
    egressChunked_(false),
    inChunk_(false),
    lastChunkWritten_(false),
    keepalive_(true),
    disableKeepalivePending_(false),
    connectRequest_(false),
    headRequest_(false),
    expectNoResponseBody_(false),
    mayChunkEgress_(false),
    is1xxResponse_(false),
    inRecvLastChunk_(false),
    headersComplete_(false) {
  switch (direction) {
  case TransportDirection::DOWNSTREAM:
    http_parser_init(&parser_, HTTP_REQUEST);
    break;
  case TransportDirection::UPSTREAM:
    http_parser_init(&parser_, HTTP_RESPONSE);
    break;
  }
  parser_.data = this;
}

void HTTP1xCodec::setParserPaused(bool paused) {
  if ((paused == parserPaused_) || parserError_) {
    // If we're bailing early, we better be paused already
    DCHECK(parserError_ ||
           (HTTP_PARSER_ERRNO(&parser_) == HPE_PAUSED) == paused);
    return;
  }
  if (paused) {
    if (HTTP_PARSER_ERRNO(&parser_) == HPE_OK) {
      http_parser_pause(&parser_, 1);
    }
  } else {
    http_parser_pause(&parser_, 0);
  }
  parserPaused_ = paused;
}

size_t HTTP1xCodec::onIngress(const acc::IOBuf& buf) {
  if (parserError_) {
    return 0;
  }
  // Callers responsibility to prevent calling onIngress from a callback
  ACCCHECK(!parserActive_);
  parserActive_ = true;
  currentIngressBuf_ = &buf;
  size_t bytesParsed = http_parser_execute(&parser_,
                                           &kParserSettings,
                                           (const char*)buf.data(),
                                           buf.length());
  // in case we parsed a section of the headers but we're not done parsing
  // the headers we need to keep accounting of it for total header size
  if (!headersComplete_) {
    headerSize_.uncompressed += bytesParsed;
  }
  parserActive_ = false;
  parserError_ = (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) &&
                 (HTTP_PARSER_ERRNO(&parser_) != HPE_PAUSED);
  if (parserError_) {
    onParserError();
  }
  if (currentHeaderName_.empty() && !currentHeaderNameStringPiece_.empty()) {
    // we currently are storing a chunk of header name via pointers in
    // currentHeaderNameacc::StringPiece_, but the currentIngressBuf_ is about to
    // vanish and so we need to copy over that data to currentHeaderName_
    currentHeaderName_.assign(currentHeaderNameStringPiece_.begin(),
                              currentHeaderNameStringPiece_.size());
  }
  currentIngressBuf_ = nullptr;
  if (pendingEOF_) {
    onIngressEOF();
    pendingEOF_ = false;
  }
  return bytesParsed;
}

void HTTP1xCodec::onIngressEOF() {
  if (parserError_) {
    return;
  }
  if (parserActive_) {
    pendingEOF_ = true;
    return;
  }
  parserActive_ = true;
  if (http_parser_execute(&parser_, &kParserSettings, nullptr, 0) != 0) {
    parserError_ = true;
  } else {
    parserError_ = (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) &&
        (HTTP_PARSER_ERRNO(&parser_) != HPE_PAUSED);
  }
  parserActive_ = false;
  if (parserError_) {
    onParserError();
  }
}

void HTTP1xCodec::onParserError(const char* what) {
  inRecvLastChunk_ = false;
  http_errno parser_errno = HTTP_PARSER_ERRNO(&parser_);
  HTTPException error(HTTPException::Direction::INGRESS,
                      what ? what : acc::to<std::string>(
                        "Error parsing message: ",
                        http_errno_description(parser_errno)
                      ));
  // generate a string of parsed headers so that we can pass it to callback
  if (msg_) {
    error.setPartialMsg(std::move(msg_));
  }
  // store the ingress buffer
  if (currentIngressBuf_) {
    error.setCurrentIngressBuf(std::move(currentIngressBuf_->clone()));
  }
  // See http_parser.h for what these error codes mean
  if (parser_errno == HPE_INVALID_EOF_STATE) {
    error.setNetError(kErrorEOF);
  } else if (parser_errno == HPE_HEADER_OVERFLOW ||
             parser_errno == HPE_INVALID_CONSTANT ||
             (parser_errno >= HPE_INVALID_VERSION &&
              parser_errno <= HPE_HUGE_CONTENT_LENGTH)) {
    error.setNetError(kErrorParseHeader);
  } else if (parser_errno == HPE_INVALID_CHUNK_SIZE ||
             parser_errno == HPE_HUGE_CHUNK_SIZE) {
    error.setNetError(kErrorParseBody);
  } else {
    error.setNetError(kErrorUnknown);
  }
  callback_->onError(error);
}

bool HTTP1xCodec::isReusable() const {
  return keepalive_;
}

bool HTTP1xCodec::isBusy() const {
  return requestPending_ || responsePending_;
}

void HTTP1xCodec::addDateHeader(acc::IOBufQueue& writeBuf, size_t& len) {
  appendLiteral(writeBuf, len, "Date: ");
  appendString(writeBuf, len, HTTPMessage::formatDateHeader());
  appendLiteral(writeBuf, len, CRLF);
}

void HTTP1xCodec::generateHeader(acc::IOBufQueue& writeBuf,
                                 const HTTPMessage& msg,
                                 bool eom,
                                 HTTPHeaderSize* size) {
  if (keepalive_ && disableKeepalivePending_) {
    keepalive_ = false;
  }
  const bool upstream = (transportDirection_ == TransportDirection::UPSTREAM);
  const bool downstream = !upstream;
  if (upstream) {
    requestPending_ = true;
    responsePending_ = true;
    connectRequest_ = (msg.getMethod() == HTTPMethod::CONNECT);
    headRequest_ = (msg.getMethod() == HTTPMethod::HEAD);
    expectNoResponseBody_ = connectRequest_ || headRequest_;
  } else {
    is1xxResponse_ = msg.is1xxResponse();

    expectNoResponseBody_ =
      connectRequest_ || headRequest_ ||
      RFC2616::responseBodyMustBeEmpty(msg.getStatusCode());
  }

  egressChunked_ = msg.getIsChunked();
  lastChunkWritten_ = false;
  std::pair<uint8_t, uint8_t> version = msg.getHTTPVersion();
  if (version > HTTPMessage::kHTTPVersion11) {
    version = HTTPMessage::kHTTPVersion11;
  }

  size_t len = 0;
  switch (transportDirection_) {
  case TransportDirection::DOWNSTREAM:
    appendLiteral(writeBuf, len, "HTTP/");
    appendUint(writeBuf, len, version.first);
    appendLiteral(writeBuf, len, ".");
    appendUint(writeBuf, len, version.second);
    appendLiteral(writeBuf, len, " ");
    appendUint(writeBuf, len, msg.getStatusCode());
    appendLiteral(writeBuf, len, " ");
    appendString(writeBuf, len, msg.getStatusMessage());
    break;
  case TransportDirection::UPSTREAM:
    if (forceUpstream1_1_ && version < HTTPMessage::kHTTPVersion11) {
      version = HTTPMessage::kHTTPVersion11;
    }
    appendString(writeBuf, len, msg.getMethodString());
    appendLiteral(writeBuf, len, " ");
    appendString(writeBuf, len, msg.getURL());
    appendLiteral(writeBuf, len, " HTTP/");
    appendUint(writeBuf, len, version.first);
    appendLiteral(writeBuf, len, ".");
    appendUint(writeBuf, len, version.second);
    mayChunkEgress_ = (version.first == 1) && (version.second >= 1);
    break;
  }
  if (keepalive_ &&
      (!msg.wantsKeepalive() ||
       version.first < 1 ||
       (downstream && version == HTTPMessage::kHTTPVersion10 &&
        keepaliveRequested_ != KeepaliveRequested::ENABLED))) {
    // Disable keepalive if
    //  - the message asked to turn it off
    //  - it's HTTP/0.9
    //  - this is a response to a 1.0 request that didn't say keep-alive
    keepalive_ = false;
  }
  egressChunked_ &= mayChunkEgress_;
  appendLiteral(writeBuf, len, CRLF);
  const std::string* deferredContentLength = nullptr;
  bool hasTransferEncodingChunked = false;
  bool hasServerHeader = false;
  bool hasDateHeader = false;
  msg.getHeaders().forEachWithCode([&] (HTTPHeaderCode code,
                                        const std::string& header,
                                        const std::string& value) {
    if (code == HTTP_HEADER_CONTENT_LENGTH) {
      // Write the Content-Length last (t1071703)
      deferredContentLength = &value;
      return; // continue
    } else if (code == HTTP_HEADER_CONNECTION) {
      // TODO: add support for the case where "close" is part of
      // a comma-separated list of values
      static const std::string kClose = "close";
      if (acc::caseInsensitiveEqual(value, kClose)) {
        keepalive_ = false;
      }
      // We'll generate a new Connection header based on the keepalive_ state
      return;
    } else if (!hasTransferEncodingChunked &&
               code == HTTP_HEADER_TRANSFER_ENCODING) {
      static const std::string kChunked = "chunked";
      if (!acc::caseInsensitiveEqual(value, kChunked)) {
        return;
      }
      hasTransferEncodingChunked = true;
      if (!mayChunkEgress_) {
        return;
      }
    } else if (!hasServerHeader && code == HTTP_HEADER_SERVER) {
      hasServerHeader = true;
    } else if (!hasDateHeader && code == HTTP_HEADER_DATE) {
      hasDateHeader = true;
    }
    size_t lineLen = header.length() + value.length() + 4; // 4 for ": " + CRLF
    auto writable = writeBuf.preallocate(lineLen,
        std::max(lineLen, size_t(2000)));
    char* dst = (char*)writable.first;
    memcpy(dst, header.data(), header.length());
    dst += header.length();
    *dst++ = ':';
    *dst++ = ' ';
    memcpy(dst, value.data(), value.length());
    dst += value.length();
    *dst++ = '\r';
    *dst = '\n';
    DCHECK(size_t(++dst - (char*)writable.first) == lineLen);
    writeBuf.postallocate(lineLen);
    len += lineLen;
  });
  bool bodyCheck =
    (downstream && keepalive_ && !expectNoResponseBody_) ||
    // auto chunk POSTs and any request that came to us chunked
    (upstream && ((msg.getMethod() == HTTPMethod::POST) || egressChunked_));
  // TODO: 400 a 1.0 POST with no content-length
  // clear egressChunked_ if the header wasn't actually set
  egressChunked_ &= hasTransferEncodingChunked;
  if (bodyCheck && !egressChunked_ && !deferredContentLength) {
    // On a connection that would otherwise be eligible for keep-alive,
    // we're being asked to send a response message with no Content-Length,
    // no chunked encoding, and no special circumstances that would eliminate
    // the need for a response body. If the client supports chunking, turn
    // on chunked encoding now.  Otherwise, turn off keepalives on this
    // connection.
    if (!hasTransferEncodingChunked && mayChunkEgress_) {
      appendLiteral(writeBuf, len, "Transfer-Encoding: chunked\r\n");
      egressChunked_ = true;
    } else {
      keepalive_ = false;
    }
  }
  if (downstream && !hasServerHeader) {
    appendLiteral(writeBuf, len, "Server: Raster/");
    appendLiteral(writeBuf, len, RDD_VERSION);
    appendLiteral(writeBuf, len, CRLF);
  }
  if (downstream && !hasDateHeader) {
    addDateHeader(writeBuf, len);
  }
  if (!is1xxResponse_ || upstream) {
    appendLiteral(writeBuf, len, "Connection: ");
    if (keepalive_) {
      appendLiteral(writeBuf, len, "keep-alive\r\n");
    } else {
      appendLiteral(writeBuf, len, "close\r\n");
    }
  }
  if (deferredContentLength) {
    appendLiteral(writeBuf, len, "Content-Length: ");
    appendString(writeBuf, len, *deferredContentLength);
    appendLiteral(writeBuf, len, CRLF);
  }
  appendLiteral(writeBuf, len, CRLF);
  if (eom) {
    len += generateEOM(writeBuf);
  }

  if (size) {
    size->compressed = 0;
    size->uncompressed = len;
  }
}

size_t HTTP1xCodec::generateBody(acc::IOBufQueue& writeBuf,
                                 std::unique_ptr<acc::IOBuf> chain,
                                 bool eom) {
  if (!chain) {
    return 0;
  }
  size_t buflen = chain->computeChainDataLength();
  size_t totLen = buflen;
  if (totLen == 0) {
    if (eom) {
      totLen += generateEOM(writeBuf);
    }
    return totLen;
  }

  if (egressChunked_ && !inChunk_) {
    char chunkLenBuf[32];
    int rc = snprintf(chunkLenBuf, sizeof(chunkLenBuf), "%zx\r\n", buflen);
    ACCCHECK(rc > 0);
    ACCCHECK(size_t(rc) < sizeof(chunkLenBuf));

    writeBuf.append(chunkLenBuf, rc);
    totLen += rc;

    writeBuf.append(std::move(chain));
    writeBuf.append("\r\n", 2);
    totLen += 2;
  } else {
    writeBuf.append(std::move(chain));
  }
  if (eom) {
    totLen += generateEOM(writeBuf);
  }

  return totLen;
}

size_t HTTP1xCodec::generateChunkHeader(acc::IOBufQueue& writeBuf, size_t length) {
  // TODO: Format directly into the acc::IOBuf, rather than copying after the fact.
  // acc::IOBufQueue::append() currently forces us to copy.

  ACCCHECK(length) << "use sendEOM to terminate the message using the "
                << "standard zero-length chunk. Don't "
                << "send zero-length chunks using this API.";
  if (egressChunked_) {
    ACCCHECK(!inChunk_);
    inChunk_ = true;
    char chunkLenBuf[32];
    int rc = snprintf(chunkLenBuf, sizeof(chunkLenBuf), "%zx\r\n", length);
    ACCCHECK(rc > 0);
    ACCCHECK(size_t(rc) < sizeof(chunkLenBuf));

    writeBuf.append(chunkLenBuf, rc);
    return rc;
  }

  return 0;
}

size_t HTTP1xCodec::generateChunkTerminator(acc::IOBufQueue& writeBuf) {
  if (egressChunked_ && inChunk_) {
    inChunk_ = false;
    writeBuf.append("\r\n", 2);
    return 2;
  }

  return 0;
}

size_t HTTP1xCodec::generateTrailers(acc::IOBufQueue& writeBuf,
                                     const HTTPHeaders& trailers) {
  size_t len = 0;
  if (egressChunked_) {
    ACCCHECK(!inChunk_);
    appendLiteral(writeBuf, len, "0\r\n");
    lastChunkWritten_ = true;
    trailers.forEach([&] (const std::string& trailer,
                          const std::string& value) {
      appendString(writeBuf, len, trailer);
      appendLiteral(writeBuf, len, ": ");
      appendString(writeBuf, len, value);
      appendLiteral(writeBuf, len, CRLF);
    });
  }
  return len;
}

size_t HTTP1xCodec::generateEOM(acc::IOBufQueue& writeBuf) {
  size_t len = 0;
  if (egressChunked_) {
    ACCCHECK(!inChunk_);
    if (headRequest_ && transportDirection_ == TransportDirection::DOWNSTREAM) {
      lastChunkWritten_ = true;
    } else {
      // appending a 0\r\n only if it's not a HEAD and downstream request
      if (!lastChunkWritten_) {
        lastChunkWritten_ = true;
        if (!(headRequest_ &&
              transportDirection_ == TransportDirection::DOWNSTREAM)) {
          appendLiteral(writeBuf, len, "0\r\n");
        }
      }
      appendLiteral(writeBuf, len, CRLF);
    }
  }
  switch (transportDirection_) {
  case TransportDirection::DOWNSTREAM:
    responsePending_ = false;
    break;
  case TransportDirection::UPSTREAM:
    requestPending_ = false;
    break;
  }
  return len;
}

size_t HTTP1xCodec::generateAbort(acc::IOBufQueue& writeBuf) {
  // We won't be able to send anything else on the transport after this.
  disableKeepalivePending_ = true;
  return 0;
}

int HTTP1xCodec::onMessageBegin() {
  headersComplete_ = false;
  headerSize_.uncompressed = 0;
  headerParseState_ = HeaderParseState::kParsingHeaderStart;
  msg_.reset(new HTTPMessage());
  trailers_.reset();
  if (transportDirection_ == TransportDirection::DOWNSTREAM) {
    requestPending_ = true;
    responsePending_ = true;
  }
  is1xxResponse_ = false;
  callback_->onMessageBegin(msg_.get());
  return 0;
}

int HTTP1xCodec::onURL(const char* buf, size_t len) {
  url_.append(buf, len);
  return 0;
}

int HTTP1xCodec::onReason(const char* buf, size_t len) {
  reason_.append(buf, len);
  return 0;
}

void HTTP1xCodec::pushHeaderNameAndValue(HTTPHeaders& hdrs) {
  if (LIKELY(currentHeaderName_.empty())) {
    hdrs.add(currentHeaderNameStringPiece_, std::move(currentHeaderValue_));
  } else {
    hdrs.add(currentHeaderName_, std::move(currentHeaderValue_));
    currentHeaderName_.clear();
  }
  currentHeaderNameStringPiece_.clear();
  currentHeaderValue_.clear();
}

int HTTP1xCodec::onHeaderField(const char* buf, size_t len) {
  if (headerParseState_ == HeaderParseState::kParsingHeaderValue) {
    pushHeaderNameAndValue(msg_->getHeaders());
  } else if (headerParseState_ == HeaderParseState::kParsingTrailerValue) {
    if (!trailers_) {
      trailers_.reset(new HTTPHeaders());
    }
    pushHeaderNameAndValue(*trailers_);
  }

  if (isParsingHeaderOrTrailerName()) {

    // we're already parsing a header name
    if (currentHeaderName_.empty()) {
      // but we've been keeping it in currentHeaderNameacc::StringPiece_ until now
      if (currentHeaderNameStringPiece_.end() == buf) {
        // the header name we are currently reading is contiguous in memory,
        // and so we just adjust the right end of our acc::StringPiece;
        // this is likely because onIngress() hasn't been called since we got
        // the previous chunk (otherwise currentHeaderName_ would be nonempty)
        currentHeaderNameStringPiece_.advance(len);
      } else {
        // this is just for safety - if for any reason there is a discontinuity
        // even though we are during the same onIngress() call,
        // we fall back to currentHeaderName_
        currentHeaderName_.assign(currentHeaderNameStringPiece_.begin(),
                                  currentHeaderNameStringPiece_.size());
        currentHeaderName_.append(buf, len);
      }
    } else {
      // we had already fallen back to currentHeaderName_ before
      currentHeaderName_.append(buf, len);
    }

  } else {
    // we're not yet parsing a header name - this is the first chunk
    // (typically, there is only one)
    currentHeaderNameStringPiece_.reset(buf, len);

    if (headerParseState_ >= HeaderParseState::kParsingHeadersComplete) {
      headerParseState_ = HeaderParseState::kParsingTrailerName;
    } else {
      headerParseState_ = HeaderParseState::kParsingHeaderName;
    }
  }
  return 0;
}

int HTTP1xCodec::onHeaderValue(const char* buf, size_t len) {
  if (isParsingHeaders()) {
    headerParseState_ = HeaderParseState::kParsingHeaderValue;
  } else {
    headerParseState_ = HeaderParseState::kParsingTrailerValue;
  }
  currentHeaderValue_.append(buf, len);
  return 0;
}

int HTTP1xCodec::onHeadersComplete(size_t len) {
  if (headerParseState_ == HeaderParseState::kParsingHeaderValue) {
    pushHeaderNameAndValue(msg_->getHeaders());
  }

  // Update the HTTPMessage with the values parsed from the header
  msg_->setHTTPVersion(parser_.http_major, parser_.http_minor);
  msg_->setIsChunked((parser_.flags & F_CHUNKED));

  if (transportDirection_ == TransportDirection::DOWNSTREAM) {
    // Set the method type
    msg_->setMethod(http_method_str(static_cast<http_method>(parser_.method)));

    connectRequest_ = (msg_->getMethod() == HTTPMethod::CONNECT);

    // If this is a headers-only request, we shouldn't send
    // an entity-body in the response.
    headRequest_ = (msg_->getMethod() == HTTPMethod::HEAD);

    ParseURL parseUrl = msg_->setURL(std::move(url_));
    url_.clear();

    if (parseUrl.hasHost()) {
      // RFC 2616 5.2.1 states "If Request-URI is an absoluteURI, the host
      // is part of the Request-URI. Any Host header field value in the
      // request MUST be ignored."
      auto hostAndPort = parseUrl.hostAndPort();
      ACCLOG(V4) << "Adding inferred host header: " << hostAndPort;
      msg_->getHeaders().set(HTTP_HEADER_HOST, hostAndPort);
    }

    // If the client sent us an HTTP/1.x with x >= 1, we may send
    // chunked responses.
    mayChunkEgress_ = ((parser_.http_major == 1) && (parser_.http_minor >= 1));
  } else {
    msg_->setStatusCode(parser_.status_code);
    msg_->setStatusMessage(std::move(reason_));
    reason_.clear();
  }

  headerParseState_ = HeaderParseState::kParsingHeadersComplete;
  bool msgKeepalive = msg_->computeKeepalive();
  if (!msgKeepalive) {
     keepalive_ = false;
  }
  if (transportDirection_ == TransportDirection::DOWNSTREAM) {
    // Remember whether this was an HTTP 1.0 request with keepalive enabled
    if (msgKeepalive && msg_->isHTTP1_0() &&
          (keepaliveRequested_ == KeepaliveRequested::UNSET ||
           keepaliveRequested_ == KeepaliveRequested::ENABLED)) {
      keepaliveRequested_ = KeepaliveRequested::ENABLED;
    } else {
      keepaliveRequested_ = KeepaliveRequested::DISABLED;
    }
  }

  // Determine whether the HTTP parser should ignore any headers
  // that indicate the presence of a message body.  This is needed,
  // for example, if the message is a response to a request with
  // method==HEAD.
  bool ignoreBody;
  if (transportDirection_ == TransportDirection::DOWNSTREAM) {
    ignoreBody = false;
  } else {
    is1xxResponse_ = msg_->is1xxResponse();
    if (expectNoResponseBody_) {
      ignoreBody = true;
    } else {
      ignoreBody = RFC2616::responseBodyMustBeEmpty(msg_->getStatusCode());
    }
  }

  headersComplete_ = true;
  headerSize_.uncompressed += len;
  msg_->setIngressHeaderSize(headerSize_);

  callback_->onHeadersComplete(std::move(msg_));

  // 1 is a magic value that tells the http_parser not to expect a
  // message body even if the message header implied the presence
  // of one (e.g., via a Content-Length)
  return (ignoreBody) ? 1 : 0;
}

int HTTP1xCodec::onBody(const char* buf, size_t len) {
  DCHECK(!isParsingHeaders());
  DCHECK(!inRecvLastChunk_);
  ACCCHECK(currentIngressBuf_ != nullptr);
  const char* dataStart = (const char*)currentIngressBuf_->data();
  const char* dataEnd = dataStart + currentIngressBuf_->length();
  DCHECK_GE(buf, dataStart);
  DCHECK_LE(buf + len, dataEnd);
  std::unique_ptr<acc::IOBuf> clone(currentIngressBuf_->clone());
  clone->trimStart(buf - dataStart);
  clone->trimEnd(dataEnd - (buf + len));
  callback_->onBody(std::move(clone));
  return 0;
}

int HTTP1xCodec::onChunkHeader(size_t len) {
  if (len > 0) {
    callback_->onChunkHeader(len);
  } else {
    ACCLOG(V5)
      << "Suppressed onChunkHeader callback for final zero length chunk";
    inRecvLastChunk_ = true;
  }
  return 0;
}

int HTTP1xCodec::onChunkComplete() {
  if (inRecvLastChunk_) {
    inRecvLastChunk_ = false;
  } else {
    callback_->onChunkComplete();
  }
  return 0;
}

int HTTP1xCodec::onMessageComplete() {
  DCHECK(!isParsingHeaders());
  DCHECK(!inRecvLastChunk_);
  if (headerParseState_ == HeaderParseState::kParsingTrailerValue) {
    if (!trailers_) {
      trailers_.reset(new HTTPHeaders());
    }
    pushHeaderNameAndValue(*trailers_);
  }

  headerParseState_ = HeaderParseState::kParsingHeaderIdle;
  if (trailers_) {
    callback_->onTrailersComplete(std::move(trailers_));
  }

  switch (transportDirection_) {
  case TransportDirection::DOWNSTREAM:
    requestPending_ = false;
    break;
  case TransportDirection::UPSTREAM:
    responsePending_ = is1xxResponse_;
  }

  callback_->onMessageComplete();
  return 0;
}

#define HTTP1X_CODEC_PARSE(onTarget, ...) { \
  HTTP1xCodec* codec = static_cast<HTTP1xCodec*>(parser->data); \
  DCHECK(codec != nullptr); \
  DCHECK_EQ(&codec->parser_, parser); \
  try { \
    return codec->onTarget(__VA_ARGS__); \
  } catch (const std::exception& ex) { \
    codec->onParserError(ex.what()); \
  } \
}

int HTTP1xCodec::onMessageBeginCB(http_parser* parser) {
  HTTP1X_CODEC_PARSE(onMessageBegin);
  return 1;
}

int HTTP1xCodec::onUrlCB(http_parser* parser, const char* buf, size_t len) {
  HTTP1X_CODEC_PARSE(onURL, buf, len);
  return 1;
}

int HTTP1xCodec::onReasonCB(http_parser* parser, const char* buf, size_t len) {
  HTTP1X_CODEC_PARSE(onReason, buf, len);
  return 1;
}

int HTTP1xCodec::onHeaderFieldCB(http_parser* parser,
                                 const char* buf, size_t len) {
  HTTP1X_CODEC_PARSE(onHeaderField, buf, len);
  return 1;
}

int HTTP1xCodec::onHeaderValueCB(http_parser* parser,
                                 const char* buf, size_t len) {
  HTTP1X_CODEC_PARSE(onHeaderValue, buf, len);
  return 1;
}

int HTTP1xCodec::onHeadersCompleteCB(http_parser* parser,
                                     const char* buf, size_t len) {
  HTTP1X_CODEC_PARSE(onHeadersComplete, len);
  return 3;
}

int HTTP1xCodec::onBodyCB(http_parser* parser, const char* buf, size_t len) {
  // Note: http_parser appears to completely ignore the return value from the
  // on_body() callback.  There seems to be no way to abort parsing after an
  // error in on_body().
  //
  // We handle this by checking if error_ is set after each call to
  // http_parser_execute().
  HTTP1X_CODEC_PARSE(onBody, buf, len);
  return 1;
}

int HTTP1xCodec::onChunkHeaderCB(http_parser* parser) {
  HTTP1X_CODEC_PARSE(onChunkHeader, parser->content_length);
  return 1;
}

int HTTP1xCodec::onChunkCompleteCB(http_parser* parser) {
  HTTP1X_CODEC_PARSE(onChunkComplete);
  return 1;
}

int HTTP1xCodec::onMessageCompleteCB(http_parser* parser) {
  HTTP1X_CODEC_PARSE(onMessageComplete);
  return 1;
}

void HTTP1xCodec::initParserSettings() {
  kParserSettings.on_message_begin = HTTP1xCodec::onMessageBeginCB;
  kParserSettings.on_url = HTTP1xCodec::onUrlCB;
  kParserSettings.on_reason = HTTP1xCodec::onReasonCB;
  kParserSettings.on_header_field = HTTP1xCodec::onHeaderFieldCB;
  kParserSettings.on_header_value = HTTP1xCodec::onHeaderValueCB;
  kParserSettings.on_headers_complete = HTTP1xCodec::onHeadersCompleteCB;
  kParserSettings.on_body = HTTP1xCodec::onBodyCB;
  kParserSettings.on_chunk_header = HTTP1xCodec::onChunkHeaderCB;
  kParserSettings.on_chunk_complete = HTTP1xCodec::onChunkCompleteCB;
  kParserSettings.on_message_complete = HTTP1xCodec::onMessageCompleteCB;
}

} // namespace rdd
