/*
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

  ~HTTPTransport() override {}

  void reset() override;

  void processReadData() override;

  void sendHeaders(const HTTPMessage& headers, HTTPHeaderSize* size);
  size_t sendBody(std::unique_ptr<acc::IOBuf> body, bool includeEOM);
  size_t sendChunkHeader(size_t length);
  size_t sendChunkTerminator();
  size_t sendTrailers(const HTTPHeaders& trailers);
  size_t sendEOM();
  size_t sendAbort();

  std::unique_ptr<HTTPMessage> headers;
  std::unique_ptr<acc::IOBuf> body;
  std::unique_ptr<HTTPHeaders> trailers;

 private:
  // HTTP1xCodec::Callback
  void onMessageBegin(HTTPMessage* msg) override;
  void onHeadersComplete(std::unique_ptr<HTTPMessage> msg) override;
  void onBody(std::unique_ptr<acc::IOBuf> chain) override;
  void onChunkHeader(size_t length) override;
  void onChunkComplete() override;
  void onTrailersComplete(std::unique_ptr<HTTPHeaders> trailers) override;
  void onMessageComplete() override;
  void onError(const HTTPException& error) override;

  HTTP1xCodec codec_;
};

class HTTPTransportFactory : public TransportFactory {
 public:
  HTTPTransportFactory(TransportDirection direction)
    : direction_(direction) {}

  ~HTTPTransportFactory() override {}

  std::unique_ptr<Transport> create() override {
    return acc::make_unique<HTTPTransport>(direction_);
  }

 private:
  TransportDirection direction_;
};

} // namespace rdd
