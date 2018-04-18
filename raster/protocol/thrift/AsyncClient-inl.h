/*
 * Copyright 2018 Yeolar
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

#include <arpa/inet.h>

#include "accelerator/Logging.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/thrift/Util.h"

namespace rdd {

template <class C>
TAsyncClient<C>::TAsyncClient(const ClientOption& option)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), option) {
  init();
}

template <class C>
TAsyncClient<C>::TAsyncClient(const Peer& peer,
                           const TimeoutOption& timeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(), peer, timeout) {
  init();
}

template <class C>
TAsyncClient<C>::TAsyncClient(const Peer& peer,
                           uint64_t ctimeout,
                           uint64_t rtimeout,
                           uint64_t wtimeout)
  : AsyncClient(acc::Singleton<HubAdaptor>::try_get(),
                peer, ctimeout, rtimeout, wtimeout) {
  init();
}

template <class C>
template <class Res>
bool TAsyncClient<C>::recv(void (C::*recvFunc)(Res&), Res& response) {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  auto range = transport->body->coalesce();
  pibuf_->resetBuffer((uint8_t*)range.data(), range.size());

  if (keepalive_) {
    int32_t seqid = thrift::getSeqId(pibuf_.get());
    if (seqid != event_->seqid()) {
      ACCLOG(ERROR) << "peer[" << peer_ << "]"
        << " recv unmatched seqid: " << seqid << "!=" << event_->seqid();
      event_->setState(Event::kFail);
    }
  }
  (client_.get()->*recvFunc)(response);
  return true;
}

template <class C>
template <class Res>
bool TAsyncClient<C>::recv(Res (C::*recvFunc)(void), Res& response) {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  auto range = transport->body->coalesce();
  pibuf_->resetBuffer((uint8_t*)range.data(), range.size());

  if (keepalive_) {
    int32_t seqid = thrift::getSeqId(pibuf_.get());
    if (seqid != event_->seqid()) {
      ACCLOG(ERROR) << "peer[" << peer_ << "]"
        << " recv unmatched seqid: " << seqid << "!=" << event_->seqid();
      event_->setState(Event::kFail);
    }
  }
  response = (client_.get()->*recvFunc)();
  return true;
}

template <class C>
template <class... Req>
bool TAsyncClient<C>::send(
    void (C::*sendFunc)(const Req&...), const Req&... requests) {
  if (!event_) {
    return false;
  }
  (client_.get()->*sendFunc)(requests...);

  if (keepalive_) {
    thrift::setSeqId(pobuf_.get(), event_->seqid());
  }
  uint8_t* p;
  uint32_t n;
  pobuf_->getBuffer(&p, &n);
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(n);
  transport->sendBody(acc::IOBuf::copyBuffer(p, n));;
  return true;
}

template <class C>
template <class Res, class... Req>
bool TAsyncClient<C>::fetch(
    void (C::*recvFunc)(Res&), Res& response,
    void (C::*sendFunc)(const Req&...), const Req&... requests) {
  return (send(sendFunc, requests...) &&
          FiberManager::yield() &&
          recv(recvFunc, response));
}

template <class C>
template <class Res, class... Req>
bool TAsyncClient<C>::fetch(
    Res (C::*recvFunc)(void), Res& response,
    void (C::*sendFunc)(const Req&...), const Req&... requests) {
  return (send(sendFunc, requests...) &&
          FiberManager::yield() &&
          recv(recvFunc, response));
}

template <class C>
std::shared_ptr<Channel> TAsyncClient<C>::makeChannel() {
  return std::make_shared<Channel>(
      peer_, timeout_, acc::make_unique<BinaryTransportFactory>());
}

template <class C>
void TAsyncClient<C>::init() {
  pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
  poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));

  client_ = acc::make_unique<C>(piprot_, poprot_);
  channel_ = makeChannel();
}

} // namespace rdd
