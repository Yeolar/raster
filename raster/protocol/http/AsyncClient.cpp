/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/AsyncClient.h"

#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/http/Transport.h"

namespace rdd {

HTTPAsyncClient::HTTPAsyncClient(const ClientOption& option)
  : AsyncClient(Singleton<HubAdaptor>::try_get(), option) {
  channel_ = makeChannel();
}

HTTPAsyncClient::HTTPAsyncClient(const Peer& peer,
                                 const TimeoutOption& timeout)
  : AsyncClient(Singleton<HubAdaptor>::try_get(), peer, timeout) {
  channel_ = makeChannel();
}

HTTPAsyncClient::HTTPAsyncClient(const Peer& peer,
                                 uint64_t ctimeout,
                                 uint64_t rtimeout,
                                 uint64_t wtimeout)
  : AsyncClient(Singleton<HubAdaptor>::try_get(),
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
                           std::unique_ptr<IOBuf> body) {
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
                            std::unique_ptr<IOBuf> body) {
  return (send(headers, std::move(body)) &&
          FiberManager::yield() &&
          recv());
}

HTTPMessage* HTTPAsyncClient::message() const {
  return event_->transport<HTTPTransport>()->headers.get();
}

IOBuf* HTTPAsyncClient::body() const {
  return event_->transport<HTTPTransport>()->body.get();
}

HTTPHeaders* HTTPAsyncClient::trailers() const {
  return event_->transport<HTTPTransport>()->trailers.get();
}

std::shared_ptr<Channel> HTTPAsyncClient::makeChannel() {
  return std::make_shared<Channel>(
      peer_,
      timeout_,
      make_unique<HTTPTransportFactory>(TransportDirection::UPSTREAM));
}

} // namespace rdd
