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

#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/proto/RpcController.h"

namespace rdd {

template <class C>
PBAsyncClient<C>::PBAsyncClient(const ClientOption& option)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), option) {
  init();
}

template <class C>
PBAsyncClient<C>::PBAsyncClient(const Peer& peer,
                             const TimeoutOption& timeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), peer, timeout) {
  init();
}

template <class C>
PBAsyncClient<C>::PBAsyncClient(const Peer& peer,
                             uint64_t ctimeout,
                             uint64_t rtimeout,
                             uint64_t wtimeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(),
                peer, ctimeout, rtimeout, wtimeout) {
  init();
}

template <class C>
bool PBAsyncClient<C>::recv() {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  rpcChannel_->process(transport->body);
  return true;
}

template <class C>
template <class Res, class Req>
bool PBAsyncClient<C>::send(
    void (C::*func)(google::protobuf::RpcController*,
                    const Req*, Res*,
                    google::protobuf::Closure*),
    Res& response,
    const Req& request) {
  if (!event_) {
    return false;
  }
  (service_.get()->*func)(controller_.get(), &request, &response, nullptr);
  return true;
}

template <class C>
template <class Res, class Req>
bool PBAsyncClient<C>::fetch(
    void (C::*func)(google::protobuf::RpcController*,
                    const Req*, Res*,
                    google::protobuf::Closure*),
    Res& response,
    const Req& request) {
  return (send(func, response, request) &&
          FiberManager::yield() &&
          recv());
}

template <class C>
std::shared_ptr<Channel> PBAsyncClient<C>::makeChannel() {
  return std::make_shared<Channel>(
      peer_, timeout_, acc::make_unique<BinaryTransportFactory>());
}

template <class C>
void PBAsyncClient<C>::init() {
  rpcChannel_.reset(new PBAsyncRpcChannel(event()));
  controller_.reset(new PBRpcController());
  service_.reset(new C(rpcChannel_.get()));
  channel_ = makeChannel();
}

} // namespace rdd
