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

#include "accelerator/Logging.h"

namespace rdd {

template <class C, class TTransport, class TProtocol>
TSyncClient<C, TTransport, TProtocol>::
TSyncClient(const ClientOption& option)
  : peer_(option.peer), timeout_(option.timeout) {
  init();
}

template <class C, class TTransport, class TProtocol>
TSyncClient<C, TTransport, TProtocol>::
TSyncClient(const Peer& peer,
                         const TimeoutOption& timeout)
  : peer_(peer), timeout_(timeout) {
  init();
}

template <class C, class TTransport, class TProtocol>
TSyncClient<C, TTransport, TProtocol>::
TSyncClient(const Peer& peer,
                         uint64_t ctimeout,
                         uint64_t rtimeout,
                         uint64_t wtimeout)
  : peer_(peer), timeout_({ctimeout, rtimeout, wtimeout}) {
  init();
}

template <class C, class TTransport, class TProtocol>
TSyncClient<C, TTransport, TProtocol>::
~TSyncClient() {
  close();
}

template <class C, class TTransport, class TProtocol>
void TSyncClient<C, TTransport, TProtocol>::
close() {
  if (transport_->isOpen()) {
    transport_->close();
  }
}

template <class C, class TTransport, class TProtocol>
bool TSyncClient<C, TTransport, TProtocol>::
connect() {
  try {
    transport_->open();
  }
  catch (apache::thrift::TException& e) {
    ACCLOG(ERROR) << "TSyncClient: connect " << peer_
      << " failed, " << e.what();
    return false;
  }
  ACCLOG(DEBUG) << "connect peer[" << peer_ << "]";
  return true;
}

template <class C, class TTransport, class TProtocol>
bool TSyncClient<C, TTransport, TProtocol>::
connected() const {
  return transport_->isOpen();
}

template <class C, class TTransport, class TProtocol>
template <class Res, class... Req>
bool TSyncClient<C, TTransport, TProtocol>::
fetch(void (C::*func)(Res&, const Req&...),
      Res& response, const Req&... requests) {
  try {
    (client_.get()->*func)(response, requests...);
  }
  catch (apache::thrift::TException& e) {
    ACCLOG(ERROR) << "TSyncClient: fetch " << peer_
      << " failed, " << e.what();
    return false;
  }
  return true;
}

template <class C, class TTransport, class TProtocol>
template <class Res, class... Req>
bool TSyncClient<C, TTransport, TProtocol>::
fetch(Res (C::*func)(const Req&...),
      Res& response, const Req&... requests) {
  try {
    response = (client_.get()->*func)(requests...);
  }
  catch (apache::thrift::TException& e) {
    ACCLOG(ERROR) << "TSyncClient: fetch " << peer_
      << " failed, " << e.what();
    return false;
  }
  return true;
}

template <class C, class TTransport, class TProtocol>
void TSyncClient<C, TTransport, TProtocol>::
init() {
  using apache::thrift::transport::TSocket;
  socket_.reset(new TSocket(peer_.getHostStr(), peer_.port()));
  socket_->setConnTimeout(timeout_.ctimeout);
  socket_->setRecvTimeout(timeout_.rtimeout);
  socket_->setSendTimeout(timeout_.wtimeout);
  transport_.reset(new TTransport(socket_));
  protocol_.reset(new TProtocol(transport_));
  client_ = acc::make_unique<C>(protocol_);
  ACCLOG(DEBUG) << "SyncClient: " << peer_
    << ", timeout=" << timeout_;
}

} // namespace rdd
