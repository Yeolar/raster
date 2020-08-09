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

#include "raster/protocol/http/AsyncClient.h"

#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/http/Transport.h"

namespace raster {

HTTPAsyncClient::HTTPAsyncClient(const ClientOption& option)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), option) {
  channel_ = makeChannel();
}

HTTPAsyncClient::HTTPAsyncClient(const Peer& peer,
                                 const TimeoutOption& timeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), peer, timeout) {
  channel_ = makeChannel();
}

HTTPAsyncClient::HTTPAsyncClient(const Peer& peer,
                                 uint64_t ctimeout,
                                 uint64_t rtimeout,
                                 uint64_t wtimeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(),
                peer, ctimeout, rtimeout, wtimeout) {
  channel_ = makeChannel();
}

bool HTTPAsyncClient::recv() {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  return true;
}

bool HTTPAsyncClient::send(const HTTPMessage& headers,
                           std::unique_ptr<acc::IOBuf> body) {
  if (!event_) {
    return false;
  }
  auto transport = event_->transport<HTTPTransport>();
  transport->sendHeaders(headers, nullptr);
  transport->sendBody(std::move(body), false);
  transport->sendEOM();
  return true;
}

bool HTTPAsyncClient::fetch(const HTTPMessage& headers,
                            std::unique_ptr<acc::IOBuf> body) {
  return (send(headers, std::move(body)) &&
          FiberManager::yield() &&
          recv());
}

HTTPMessage* HTTPAsyncClient::message() const {
  return event_->transport<HTTPTransport>()->headers.get();
}

acc::IOBuf* HTTPAsyncClient::body() const {
  return event_->transport<HTTPTransport>()->body.get();
}

HTTPHeaders* HTTPAsyncClient::trailers() const {
  return event_->transport<HTTPTransport>()->trailers.get();
}

std::shared_ptr<Channel> HTTPAsyncClient::makeChannel() {
  return std::make_shared<Channel>(
      peer_,
      timeout_,
      std::make_unique<HTTPTransportFactory>(TransportDirection::UPSTREAM));
}

} // namespace raster
