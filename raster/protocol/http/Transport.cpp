/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/protocol/http/Transport.h"

namespace raster {

void HTTPTransport::reset() {
}

void HTTPTransport::processReadData() {
  const acc::IOBuf* buf;
  while ((buf = readBuf_.front()) != nullptr && buf->length() != 0) {
    size_t bytesParsed = codec_.onIngress(*buf);
    if (bytesParsed == 0) {
      break;
    }
    readBuf_.trimStart(bytesParsed);
  }
}

#if 0
void HTTPTransport::onReadingHeaders() {
  ACCCHECK(state_ == INIT);

  acc::IOBuf* buf = rbuf.get();
  headerSize_ = buf->computeChainDataLength();

  acc::StringPiece data(buf->coalesce());
  auto eol = data.find("\r\n");
  acc::StringPiece method, uri, version;

  if (!split(' ', data.subpiece(0, eol), method, uri, version)) {
    ACCLOG(INFO) << *this << " malformed HTTP request line";
    state_ = ERROR;
    return;
  }
  if (!version.startsWith("HTTP/")) {
    ACCLOG(INFO) << *this << " malformed HTTP version in HTTP request";
    state_ = ERROR;
    return;
  }

  HTTPHeaders headers;
  headers.parse(data.subpiece(eol + 2));

  response_ = std::make_shared<HTTPResponse>();
  request_ = std::make_shared<HTTPRequest>(
      stringToMethod(method),
      uri,
      version,
      std::move(headers),
      true,
      peer().host);

  size_t n = request_->contentLength();
  if (n != 0) {
    if (n > Protocol::BODYLEN_LIMIT) {
      ACCLOG(WARN) << *this << " big request, bodyLength=" << n;
    } else {
      ACCLOG(V3) << *this << " bodyLength=" << n;
    }
    /* TODO
    if (request_->headers.get("Expect") == "100-continue") {
      wbuf->append("HTTP/1.1 100 (Continue)\r\n\r\n");
    }*/
    rlen += n;
    state_ = ON_READING;
  } else {
    state_ = ON_READING_FINISH;
  }
}

void HTTPTransport::onReadingBody() {
  ACCCHECK(state_ == ON_READING);

  acc::IOBuf* buf = rbuf.get();
  acc::StringPiece data(buf->coalesce());
  request_->body = data.subpiece(headerSize_);

  if (request_->method == HTTPMethod::POST ||
      request_->method == HTTPMethod::PUT) {
    /*
    parseBodyArguments(
        request_->headers.getSingleOrEmpty(HTTP_HEADER_CONTENT_TYPE),
        data, request_->arguments, request_->files);
        */
  }
  state_ = ON_READING_FINISH;
}

void HTTPTransport::onWriting() {
  ACCCHECK(state_ == ON_WRITING);

  if (response_->statusCode == 200 &&
      (request_->method == HTTPMethod::GET ||
       request_->method == HTTPMethod::HEAD) &&
      !response_->headers.exists(HTTP_HEADER_ETAG)) {
    auto etag = response_->computeEtag();
    if (!etag.empty()) {
      response_->headers.set(HTTP_HEADER_ETAG, etag);
      auto inm = request_->headers.combine(HTTP_HEADER_IF_NONE_MATCH);
      if (!inm.empty() && inm.find(etag) != std::string::npos) {
        response_->data->clear();
        response_->statusCode = 304;
      }
    }
  }
  if (response_->statusCode == 304) {
    ACCCHECK(response_->data->empty());
    response_->headers.clearHeadersFor304();
  } else if (!response_->headers.exists(HTTP_HEADER_CONTENT_LENGTH)) {
    size_t n = response_->data->computeChainDataLength();
    response_->headers.set(HTTP_HEADER_CONTENT_LENGTH, to<std::string>(n));
  }

  if (request_->method == HTTPMethod::HEAD) {
    response_->data->clear();
  }
  response_->prependHeaders(request_->version);
  wbuf.swap(response_->data);

  auto level = response_->statusCode < 400 ? ::raster::logging::LOG_INFO :
               response_->statusCode < 500 ? ::raster::logging::LOG_WARN :
                                             ::raster::logging::LOG_ERROR;
  ACCLOG_STREAM(level)
    << response_->statusCode << " "
    << *request_ << " "
    << this->cost()/1000.0 << "ms";

  state_ = ON_WRITING_FINISH;
}
#endif

void HTTPTransport::onMessageBegin(HTTPMessage* msg) {
  msg->setClientAddress(peerAddr_);
  msg->setDstAddress(localAddr_);
}

void HTTPTransport::onHeadersComplete(std::unique_ptr<HTTPMessage> msg) {
  headers = std::move(msg);
}

void HTTPTransport::onBody(std::unique_ptr<acc::IOBuf> chain) {
  if (body) {
    body->appendChain(std::move(chain));
  } else {
    body = std::move(chain);
  }
}

void HTTPTransport::onChunkHeader(size_t length) {}

void HTTPTransport::onChunkComplete() {}

void HTTPTransport::onTrailersComplete(std::unique_ptr<HTTPHeaders> trailers_) {
  trailers = std::move(trailers_);
}

void HTTPTransport::onMessageComplete() {
  state_ = kFinish;
}

void HTTPTransport::onError(const HTTPException& error) {
  state_ = kError;
}

void HTTPTransport::sendHeaders(const HTTPMessage& headers,
                                HTTPHeaderSize* size) {
  codec_.generateHeader(writeBuf_, headers, false, size);
}

size_t HTTPTransport::sendBody(std::unique_ptr<acc::IOBuf> body, bool includeEOM) {
  return codec_.generateBody(writeBuf_, std::move(body), includeEOM);
}

size_t HTTPTransport::sendChunkHeader(size_t length) {
  return codec_.generateChunkHeader(writeBuf_, length);
}

size_t HTTPTransport::sendChunkTerminator() {
  return codec_.generateChunkTerminator(writeBuf_);
}

size_t HTTPTransport::sendTrailers(const HTTPHeaders& trailers) {
  return codec_.generateTrailers(writeBuf_, trailers);
}

size_t HTTPTransport::sendEOM() {
  return codec_.generateEOM(writeBuf_);
}

size_t HTTPTransport::sendAbort() {
  return codec_.generateAbort(writeBuf_);
}

} // namespace raster
