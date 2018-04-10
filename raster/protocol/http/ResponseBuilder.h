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

#pragma once

#include "accelerator/io/IOBuf.h"
#include "raster/protocol/http/HTTPMessage.h"
#include "raster/protocol/http/Transport.h"
#include "accelerator/Memory.h"
#include "accelerator/ScopeGuard.h"

namespace rdd {

/**
 * Helps you make responses and send them on demand.
 *
 * Three expected use cases are
 *
 * 1. Send all response at once. If this is an error
 *    response, most probably you also want 'closeConnection'.
 *
 * ResponseBuilder(handler)
 *    .status(200, "OK")
 *    .body(...)
 *    .sendWithEOM();
 *
 * 2. Sending back response in chunks.
 *
 * ResponseBuilder(handler)
 *    .status(200, "OK")
 *    .body(...)
 *    .send();  // Without `WithEOM` we make it chunked
 *
 * 1 or more time
 *
 * ResponseBuilder(handler)
 *    .body(...)
 *    .send();
 *
 * At last
 *
 * ResponseBuilder(handler)
 *    .body(...)
 *    .sendWithEOM();
 *
 * 3. Reject Upgrade Requests
 *
 * ResponseBuilder(handler)
 *    .rejectUpgradeRequest() // send '400 Bad Request'
 *
 */
class ResponseBuilder {
 public:
  ResponseBuilder() {}

  void setupTransport(HTTPTransport* transport) {
    transport_ = transport;
  }

  ResponseBuilder& status(uint16_t code, std::string message) {
    headers_ = acc::make_unique<HTTPMessage>();
    headers_->setHTTPVersion(1, 1);
    headers_->setStatusCode(code);
    headers_->setStatusMessage(message);
    return *this;
  }

  ResponseBuilder& status(uint16_t code) {
    return status(code, HTTPMessage::getDefaultReason(code));
  }

  template <typename T>
  ResponseBuilder& header(const std::string& headerIn, const T& value) {
    ACCCHECK(headers_) << "You need to call `status` before adding headers";
    headers_->getHeaders().add(headerIn, value);
    return *this;
  }

  template <typename T>
  ResponseBuilder& header(HTTPHeaderCode code, const T& value) {
    ACCCHECK(headers_) << "You need to call `status` before adding headers";
    headers_->getHeaders().add(code, value);
    return *this;
  }

  ResponseBuilder& body(std::unique_ptr<acc::IOBuf> bodyIn) {
    if (bodyIn) {
      if (body_) {
        body_->prependChain(std::move(bodyIn));
      } else {
        body_ = std::move(bodyIn);
      }
    }
    return *this;
  }

  template <typename T>
  ResponseBuilder& body(T&& t) {
    return body(acc::IOBuf::maybeCopyBuffer(acc::to<std::string>(std::forward<T>(t))));
  }

  ResponseBuilder& closeConnection() {
    return header(HTTP_HEADER_CONNECTION, "close");
  }

  void sendWithEOM() {
    sendEOM_ = true;
    send();
  }

  void send() {
    // Once we send them, we don't want to send them again
    SCOPE_EXIT { headers_.reset(); };

    // If we have complete response, we can use Content-Length and get done
    bool chunked = !(headers_ && sendEOM_);

    if (headers_) {
      // We don't need to add Content-Length or Encoding for 1xx responses
      if (headers_->getStatusCode() >= 200) {
        if (chunked) {
          headers_->setIsChunked(true);
        } else {
          const auto len = body_ ? body_->computeChainDataLength() : 0;
          headers_->getHeaders().add(
              HTTP_HEADER_CONTENT_LENGTH,
              acc::to<std::string>(len));
        }
      }
      transport_->sendHeaders(*headers_, nullptr);
    }

    if (body_) {
      if (chunked) {
        transport_->sendChunkHeader(body_->computeChainDataLength());
        transport_->sendBody(std::move(body_), false);
        transport_->sendChunkTerminator();
      } else {
        transport_->sendBody(std::move(body_), false);
      }
    }

    if (sendEOM_) {
      transport_->sendEOM();
    }
  }

  void rejectUpgradeRequest() {
    headers_ = acc::make_unique<HTTPMessage>();
    headers_->constructDirectResponse({1, 1}, 400, "Bad Request");
    transport_->sendHeaders(*headers_, nullptr);
    transport_->sendEOM();
  }

 private:
  HTTPTransport* transport_{nullptr};
  std::unique_ptr<HTTPMessage> headers_;
  std::unique_ptr<acc::IOBuf> body_;
  bool sendEOM_{false};
};

} // namespace rdd
