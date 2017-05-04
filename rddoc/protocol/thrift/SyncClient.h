/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include "rddoc/net/NetUtil.h"
#include "rddoc/util/Logging.h"

namespace rdd {

template <
  class C,
  class TTransport = apache::thrift::transport::TFramedTransport,
  class TProtocol = apache::thrift::protocol::TBinaryProtocol>
class TSyncClient {
public:
  TSyncClient(const std::string& host,
              int port,
              uint64_t ctimeout = 100000,
              uint64_t rtimeout = 1000000,
              uint64_t wtimeout = 300000)
    : peer_({host, port}) {
    timeout_ = {ctimeout, rtimeout, wtimeout};
    init();
  }
  TSyncClient(const ClientOption& option)
    : peer_(option.peer)
    , timeout_(option.timeout) {
    init();
  }
  virtual ~TSyncClient() {
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
    catch (apache::thrift::TException& e) {
      RDDLOG(ERROR) << "TSyncClient: connect " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    RDDLOG(DEBUG) << "connect peer[" << peer_.str() << "]";
    return true;
  }

  bool connected() const { return transport_->isOpen(); }

  template <class Res, class... Req>
  bool fetch(void (C::*func)(Res&, const Req&...),
             Res& _return,
             const Req&... requests) {
    try {
      (client_.get()->*func)(_return, requests...);
    }
    catch (apache::thrift::TException& e) {
      RDDLOG(ERROR) << "TSyncClient: fetch " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    return true;
  }

  template <class Res, class... Req>
  bool fetch(Res (C::*func)(const Req&...),
             Res& _return,
             const Req&... requests) {
    try {
      _return = (client_.get()->*func)(requests...);
    }
    catch (apache::thrift::TException& e) {
      RDDLOG(ERROR) << "TSyncClient: fetch " << peer_.str()
        << " failed, " << e.what();
      return false;
    }
    return true;
  }

private:
  void init() {
    using apache::thrift::transport::TSocket;
    socket_.reset(new TSocket(peer_.host, peer_.port));
    socket_->setConnTimeout(timeout_.ctimeout);
    socket_->setRecvTimeout(timeout_.rtimeout);
    socket_->setSendTimeout(timeout_.wtimeout);
    transport_.reset(new TTransport(socket_));
    protocol_.reset(new TProtocol(transport_));
    client_ = std::make_shared<C>(protocol_);
    RDDLOG(DEBUG) << "SyncClient: " << peer_.str()
      << ", timeout=" << timeout_;
  }

  Peer peer_;
  TimeoutOption timeout_;
  std::shared_ptr<C> client_;
  boost::shared_ptr<apache::thrift::transport::TSocket> socket_;
  boost::shared_ptr<apache::thrift::transport::TTransport> transport_;
  boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol_;
};

}

