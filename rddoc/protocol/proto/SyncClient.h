/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <google/protobuf/service.h>
#include "rddoc/net/NetUtil.h"
#include "rddoc/protocol/proto/RpcChannel.h"
#include "rddoc/protocol/proto/RpcController.h"
#include "rddoc/util/Logging.h"

namespace rdd {

template <class C>
class PBSyncClient {
public:
  PBSyncClient(const std::string& host, int port)
    : peer_(host, port) {
    init();
  }
  PBSyncClient(const ClientOption& option)
    : peer_(option.peer) {
    init();
  }
  virtual ~PBSyncClient() {
    close();
  }

  void close() {
    if (rpcChannel_->isOpen()) {
      rpcChannel_->close();
    }
  }

  bool connect() {
    try {
      rpcChannel_->open();
    }
    catch (std::exception& e) {
      RDDLOG(ERROR) << "PBSyncClient: connect " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    RDDLOG(DEBUG) << "connect peer[" << peer_.str() << "]";
    return true;
  }

  bool connected() const { return rpcChannel_->isOpen(); }

  template <class Res, class Req>
  bool fetch(void (C::*func)(google::protobuf::RpcController*,
                             const Req*, Res*,
                             google::protobuf::Closure*),
             Res& _return,
             const Req& request) {
    try {
      (service_.get()->*func)(controller_.get(), &request, &_return, nullptr);
    }
    catch (std::exception& e) {
      RDDLOG(ERROR) << "PBSyncClient: fetch " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    return true;
  }

private:
  void init() {
    rpcChannel_.reset(new PBSyncRpcChannel(peer_));
    controller_.reset(new PBRpcController());
    service_.reset(new C(rpcChannel_.get()));
    RDDLOG(DEBUG) << "SyncClient: " << peer_.str();
  }

  Peer peer_;
  std::shared_ptr<PBSyncRpcChannel> rpcChannel_;
  std::shared_ptr<google::protobuf::RpcController> controller_;
  std::shared_ptr<C> service_;
};

}

