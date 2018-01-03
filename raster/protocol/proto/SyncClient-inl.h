/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Logging.h"

namespace rdd {

template <class C>
PBSyncClient<C>::PBSyncClient(const ClientOption& option)
  : peer_(option.peer), timeout_(option.timeout) {
  init();
}

template <class C>
PBSyncClient<C>::PBSyncClient(const Peer& peer,
                           const TimeoutOption& timeout)
  : peer_(peer), timeout_(timeout) {
  init();
}

template <class C>
PBSyncClient<C>::PBSyncClient(const Peer& peer,
                           uint64_t ctimeout,
                           uint64_t rtimeout,
                           uint64_t wtimeout)
  : peer_(peer), timeout_({ctimeout, rtimeout, wtimeout}) {
  init();
}

template <class C>
PBSyncClient<C>::~PBSyncClient() {
  close();
}

template <class C>
void PBSyncClient<C>::close() {
  if (rpcChannel_->isOpen()) {
    rpcChannel_->close();
  }
}

template <class C>
bool PBSyncClient<C>::connect() {
  try {
    rpcChannel_->open();
  }
  catch (std::exception& e) {
    RDDLOG(ERROR) << "PBSyncClient: connect " << peer_
      << " failed, " << e.what();
    return false;
  }
  RDDLOG(DEBUG) << "connect peer[" << peer_ << "]";
  return true;
}

template <class C>
bool PBSyncClient<C>::connected() const {
  return rpcChannel_->isOpen();
}

template <class C>
template <class Res, class Req>
bool PBSyncClient<C>::fetch(
    void (C::*func)(google::protobuf::RpcController*,
                    const Req*,
                    Res*,
                    google::protobuf::Closure*),
    Res& _return,
    const Req& request) {
  try {
    (service_.get()->*func)(controller_.get(), &request, &_return, nullptr);
  }
  catch (std::exception& e) {
    RDDLOG(ERROR) << "PBSyncClient: fetch " << peer_
      << " failed, " << e.what();
    return false;
  }
  return true;
}

template <class C>
void PBSyncClient<C>::init() {
  rpcChannel_.reset(new PBSyncRpcChannel(peer_, timeout_));
  controller_.reset(new PBRpcController());
  service_.reset(new C(rpcChannel_.get()));
  RDDLOG(DEBUG) << "SyncClient: " << peer_ << ", timeout=" << timeout_;
}

} // namespace rdd
