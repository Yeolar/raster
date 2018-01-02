/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/binary/AsyncClient.h"

#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

BinaryAsyncClient::BinaryAsyncClient(const ClientOption& option)
  : AsyncClient(Singleton<HubAdaptor>::try_get(), option) {
  channel_ = makeChannel();
}

BinaryAsyncClient::BinaryAsyncClient(const Peer& peer,
                                     const TimeoutOption& timeout)
  : AsyncClient(Singleton<HubAdaptor>::try_get(), peer, timeout) {
  channel_ = makeChannel();
}

BinaryAsyncClient::BinaryAsyncClient(const Peer& peer,
                                     uint64_t ctimeout,
                                     uint64_t rtimeout,
                                     uint64_t wtimeout)
  : AsyncClient(Singleton<HubAdaptor>::try_get(),
                peer, ctimeout, rtimeout, wtimeout) {
  channel_ = makeChannel();
}

bool BinaryAsyncClient::recv(ByteRange& response) {
  if (!event_ || event_->state() == Event::kFail) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  response = transport->body->coalesce();
  return true;
}

bool BinaryAsyncClient::send(const ByteRange& request) {
  if (!event_) {
    return false;
  }
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(request.size());
  transport->sendBody(IOBuf::copyBuffer(request));
  return true;
}

bool BinaryAsyncClient::fetch(ByteRange& response, const ByteRange& request) {
  return (send(request) &&
          FiberManager::yield() &&
          recv(response));
}

std::shared_ptr<Channel> BinaryAsyncClient::makeChannel() {
  return std::make_shared<Channel>(
      peer_,
      timeout_,
      make_unique<BinaryTransportFactory>());
}

} // namespace rdd
