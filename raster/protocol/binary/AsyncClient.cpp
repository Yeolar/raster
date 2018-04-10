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

#include "raster/protocol/binary/AsyncClient.h"

#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

BinaryAsyncClient::BinaryAsyncClient(const ClientOption& option)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), option) {
  channel_ = makeChannel();
}

BinaryAsyncClient::BinaryAsyncClient(const Peer& peer,
                                     const TimeoutOption& timeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), peer, timeout) {
  channel_ = makeChannel();
}

BinaryAsyncClient::BinaryAsyncClient(const Peer& peer,
                                     uint64_t ctimeout,
                                     uint64_t rtimeout,
                                     uint64_t wtimeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(),
                peer, ctimeout, rtimeout, wtimeout) {
  channel_ = makeChannel();
}

bool BinaryAsyncClient::recv(acc::ByteRange& response) {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  response = transport->body->coalesce();
  return true;
}

bool BinaryAsyncClient::send(const acc::ByteRange& request) {
  if (!event_) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(request.size());
  transport->sendBody(acc::IOBuf::copyBuffer(request));
  return true;
}

bool BinaryAsyncClient::fetch(acc::ByteRange& response, const acc::ByteRange& request) {
  return (send(request) &&
          FiberManager::yield() &&
          recv(response));
}

std::shared_ptr<Channel> BinaryAsyncClient::makeChannel() {
  return std::make_shared<Channel>(
      peer_,
      timeout_,
      acc::make_unique<BinaryTransportFactory>());
}

} // namespace rdd
