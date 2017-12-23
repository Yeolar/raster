/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>

#include "raster/net/NetUtil.h"
#include "raster/protocol/http/SyncTransport.h"
#include "raster/util/Logging.h"

namespace rdd {

class HTTPSyncClient {
public:
  HTTPSyncClient(const std::string& host,
                 int port,
                 uint64_t ctimeout = 100000,
                 uint64_t rtimeout = 1000000,
                 uint64_t wtimeout = 300000)
    : peer_(host, port),
      timeout_({ctimeout, rtimeout, wtimeout}) {
    init();
  }
  HTTPSyncClient(const ClientOption& option)
    : peer_(option.peer),
      timeout_(option.timeout) {
    init();
  }
  virtual ~HTTPSyncClient() {
    close();
  }

  void close() {
    if (transport_->isOpen()) {
      transport_->close();
    }
  }

  bool connect() {
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

  bool connected() const { return transport_->isOpen(); }

  bool fetch(const HTTPMessage& headers, std::unique_ptr<IOBuf> body) {
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

  HTTPMessage* headers() const { return transport_->headers.get(); }
  IOBuf* body() const { return transport_->body.get(); }
  HTTPHeaders* trailers() const { return transport_->trailers.get(); }

private:
  void init() {
    transport_.reset(new HTTPSyncTransport(peer_));
    RDDLOG(DEBUG) << "SyncClient: " << peer_
      << ", timeout=" << timeout_;
  }

  Peer peer_;
  TimeoutOption timeout_;
  std::shared_ptr<HTTPSyncTransport> transport_;
};

} // namespace rdd
