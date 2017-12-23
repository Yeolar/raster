/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>

#include "raster/3rd/thrift/protocol/TBinaryProtocol.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"
#include "raster/net/AsyncClient.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/thrift/Util.h"
#include "raster/util/Logging.h"

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
    : TAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~TAsyncClient() {}

  template <class Res>
  bool recv(void (C::*recvFunc)(Res&), Res& _return) {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    auto transport = event_->transport<BinaryTransport>();
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());

    if (keepalive_) {
      int32_t seqid = thrift::getSeqId(pibuf_.get());
      if (seqid != event_->seqid()) {
        RDDLOG(ERROR) << "peer[" << peer_.str() << "]"
          << " recv unmatched seqid: " << seqid << "!=" << event_->seqid();
        event_->setType(Event::FAIL);
      }
    }
    (client_.get()->*recvFunc)(_return);
    return true;
  }

  template <class Res>
  bool recv(Res (C::*recvFunc)(void), Res& _return) {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    auto transport = event_->transport<BinaryTransport>();
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());

    if (keepalive_) {
      int32_t seqid = thrift::getSeqId(pibuf_.get());
      if (seqid != event_->seqid()) {
        RDDLOG(ERROR) << "peer[" << peer_.str() << "]"
          << " recv unmatched seqid: " << seqid << "!=" << event_->seqid();
        event_->setType(Event::FAIL);
      }
    }
    _return = (client_.get()->*recvFunc)();
    return true;
  }

  template <class... Req>
  bool send(void (C::*sendFunc)(const Req&...), const Req&... requests) {
    if (!event_) {
      return false;
    }
    (client_.get()->*sendFunc)(requests...);

    if (keepalive_) {
      thrift::setSeqId(pobuf_.get(), event_->seqid());
    }
    uint8_t* p;
    uint32_t n;
    pobuf_->getBuffer(&p, &n);
    auto transport = event_->transport<BinaryTransport>();
    transport->sendHeader(n);
    transport->sendBody(IOBuf::copyBuffer(p, n));;
    return true;
  }

  template <class Res, class... Req>
  bool fetch(void (C::*recvFunc)(Res&), Res& _return,
         void (C::*sendFunc)(const Req&...), const Req&... requests) {
    return (send(sendFunc, requests...) &&
            FiberManager::yield() &&
            recv(recvFunc, _return));
  }

  template <class Res, class... Req>
  bool fetch(Res (C::*recvFunc)(void), Res& _return,
             void (C::*sendFunc)(const Req&...), const Req&... requests) {
    return (send(sendFunc, requests...) &&
            FiberManager::yield() &&
            recv(recvFunc, _return));
  }

  template <class... Req>
  bool fetchNoWait(void (C::*sendFunc)(const Req&...), const Req&... requests) {
    if (send(sendFunc, requests...)) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    return std::make_shared<Channel>(
        peer_, timeoutOpt_,
        make_unique<BinaryTransportFactory>());
  }

private:
  std::shared_ptr<C> client_;
};

} // namespace rdd
