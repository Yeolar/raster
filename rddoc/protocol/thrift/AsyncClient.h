/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>
#include <protocol/TBinaryProtocol.h>
#include <transport/TBufferTransports.h>
#include "rddoc/net/AsyncClient.h"
#include "rddoc/protocol/thrift/Encoding.h"
#include "rddoc/protocol/thrift/Protocol.h"

/*
 * callback mode:
 * (if you have a thrift TClient, and want to do some custom work)
 *
 *  class MyAsyncClient : public TAsyncClient<TClient> {
 *  public:
 *    MyAsyncClient(const ClientOption& option)
 *      : TAsyncClient<TClient>(option, true) {}  // use callback mode
 *
 *    virtual void callback() {
 *      Res res;
 *      recv(&TClient::recv_Func, res);
 *      doSomeWorkOnResult(res);  // do some custom work
 *    }
 *  };
 *
 *  MyAsyncClient* client = new MyAsyncClient(option);
 *  if (!client->connect()) {
 *    delete client;
 *  }
 *  if (!client->fetch(send_func, req)) {
 *    delete client;
 *  }
 *
 *  // when client fetch finished, the callback method
 *  // will be called atomatically.
 */
namespace rdd {

template <class C>
class TAsyncClient : public AsyncClient {
protected:
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pibuf_;
  boost::shared_ptr< ::apache::thrift::transport::TMemoryBuffer> pobuf_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TBinaryProtocol> poprot_;

public:
  TAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
    piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
    poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));

    client_ = std::make_shared<C>(piprot_, poprot_);
    channel_ = makeChannel();
  }
  TAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : TAsyncClient({{host, port}, {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~TAsyncClient() {}

  template <class Res>
  bool recv(void (C::*recvFunc)(Res&), Res& _return) {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    decodeData();
    (client_.get()->*recvFunc)(_return);
    return true;
  }

  template <class Res>
  bool recv(Res (C::*recvFunc)(void), Res& _return) {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    decodeData();
    _return = (client_.get()->*recvFunc)();
    return true;
  }

  template <class... Req>
  bool send(void (C::*sendFunc)(const Req&...), const Req&... requests) {
    if (!event_) {
      return false;
    }
    (client_.get()->*sendFunc)(requests...);
    encodeData();
    return true;
  }

  template <class Res, class... Req>
  bool fetch(void (C::*recvFunc)(Res&), Res& _return,
         void (C::*sendFunc)(const Req&...), const Req&... requests) {
    return (send(sendFunc, requests...) &&
            yieldTask() &&
            recv(recvFunc, _return));
  }

  template <class Res, class... Req>
  bool fetch(Res (C::*recvFunc)(void), Res& _return,
             void (C::*sendFunc)(const Req&...), const Req&... requests) {
    return (send(sendFunc, requests...) &&
            yieldTask() &&
            recv(recvFunc, _return));
  }

  template <class... Req>
  bool fetch(void (C::*sendFunc)(const Req&...), const Req&... requests) {
    if (send(sendFunc, requests...)) {
      Singleton<Actor>::get()->addClientTask((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    std::shared_ptr<Protocol> protocol(new TFramedProtocol());
    return std::make_shared<Channel>(peer_, timeout_opt_, protocol);
  }

  bool decodeData() {
    return rdd::thrift::decodeData(event_->rbuf(), pibuf_.get());
  }

  bool encodeData() {
    return rdd::thrift::encodeData(event_->wbuf(), pobuf_.get());
  }

private:
  std::shared_ptr<C> client_;
};

}

