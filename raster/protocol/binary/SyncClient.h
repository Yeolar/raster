/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/util/Logging.h"

namespace rdd {

class BinarySyncClient {
public:
  BinarySyncClient(const std::string& host, int port)
    : peer_(host, port) {
    init();
  }
  BinarySyncClient(const ClientOption& option)
    : peer_(option.peer) {
    init();
  }
  virtual ~BinarySyncClient() {
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
      RDDLOG(ERROR) << "BinarySyncClient: connect " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    RDDLOG(DEBUG) << "connect peer[" << peer_.str() << "]";
    return true;
  }

  bool connected() const { return transport_->isOpen(); }

  template <class Req = ByteRange, class Res = ByteRange>
  bool fetch(Res& _return, const Req& request) {
    try {
      transport_->send(request);
      transport_->recv(_return);
    }
    catch (std::exception& e) {
      RDDLOG(ERROR) << "BinarySyncClient: fetch " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    return true;
  }

private:
  void init() {
    transport_.reset(new BinaryTransport(peer_));
    RDDLOG(DEBUG) << "SyncClient: " << peer_.str();
  }

  Peer peer_;
  std::shared_ptr<BinaryTransport> transport_;
};

} // namespace rdd
