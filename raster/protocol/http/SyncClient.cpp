/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/SyncClient.h"

#include "raster/util/Logging.h"

namespace rdd {

HTTPSyncClient::HTTPSyncClient(const ClientOption& option)
  : peer_(option.peer), timeout_(option.timeout) {
  init();
}

HTTPSyncClient::HTTPSyncClient(const Peer& peer,
                               const TimeoutOption& timeout)
  : peer_(peer), timeout_(timeout) {
  init();
}

HTTPSyncClient::HTTPSyncClient(const Peer& peer,
                               uint64_t ctimeout,
                               uint64_t rtimeout,
                               uint64_t wtimeout)
  : peer_(peer), timeout_({ctimeout, rtimeout, wtimeout}) {
  init();
}

HTTPSyncClient::~HTTPSyncClient() {
  close();
}

void HTTPSyncClient::close() {
  if (transport_->isOpen()) {
    transport_->close();
  }
}

bool HTTPSyncClient::connect() {
  try {
    transport_->open();
  }
  catch (std::exception& e) {
    RDDLOG(ERROR) << "HTTPSyncClient: connect " << peer_
      << " failed, " << e.what();
    return false;
  }
  RDDLOG(DEBUG) << "connect peer[" << peer_ << "]";
  return true;
}

bool HTTPSyncClient::connected() const {
  return transport_->isOpen();
}

bool HTTPSyncClient::fetch(const HTTPMessage& headers,
                           std::unique_ptr<IOBuf> body) {
  try {
    transport_->sendHeaders(headers, nullptr);
    transport_->sendBody(std::move(body), false);
    transport_->sendEOM();
    transport_->send();
    transport_->recv();
  }
  catch (std::exception& e) {
    RDDLOG(ERROR) << "HTTPSyncClient: fetch " << peer_
      << " failed, " << e.what();
    return false;
  }
  return true;
}

HTTPMessage* HTTPSyncClient::headers() const {
  return transport_->headers.get();
}

IOBuf* HTTPSyncClient::body() const {
  return transport_->body.get();
}

HTTPHeaders* HTTPSyncClient::trailers() const {
  return transport_->trailers.get();
}

void HTTPSyncClient::init() {
  transport_.reset(new HTTPSyncTransport(peer_, timeout_));
  RDDLOG(DEBUG) << "SyncClient: " << peer_
    << ", timeout=" << timeout_;
}

} // namespace rdd
