/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/AsyncClient.h"
#include "rddoc/protocol/proto/Protocol.h"
#include "rddoc/protocol/proto/RpcChannel.h"
#include "rddoc/protocol/proto/RpcController.h"

/*
 * callback mode:
 * (if you have a pb Stub, and want to do some custom work)
 *
 *  class MyAsyncClient : public PBAsyncClient<Stub> {
 *  public:
 *    MyAsyncClient(const ClientOption& option)
 *      : PBAsyncClient<Stub>(option, true) {}  // use callback mode
 *
 *    virtual void callback() {
 *      recv();
 *    }
 *  };
 *
 *  MyAsyncClient* client = new MyAsyncClient(option);
 *  if (!client->connect()) {
 *    delete client;
 *  }
 *  if (!client->fetch(func, res, req)) {
 *    delete client;
 *  }
 *
 *  // when client fetch finished, the callback method
 *  // will be called atomatically.
 */
namespace rdd {

template <class C>
class PBAsyncClient : public AsyncClient {
public:
  PBAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    rpcChannel_.reset(new PBAsyncRpcChannel(event()));
    controller_.reset(new PBRpcController());
    service_.reset(new C(rpcChannel_.get()));
    channel_ = makeChannel();
  }
  PBAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : PBAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~PBAsyncClient() {}

  bool recv() {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    std::string msg;
    proto::decodeData(event_->rbuf(), &msg);
    rpcChannel_->process(msg);
    return true;
  }

  template <class Res, class Req>
  bool send(void (C::*func)(google::protobuf::RpcController*,
                            const Req*, Res*,
                            google::protobuf::Closure*),
            Res& _return, const Req& request) {
    if (!event_) {
      return false;
    }
    (service_.get()->*func)(controller_.get(), &request, &_return, nullptr);
    return true;
  }

  template <class Res, class Req>
  bool fetch(void (C::*func)(google::protobuf::RpcController*,
                             const Req*, Res*,
                             google::protobuf::Closure*),
             Res& _return, const Req& request) {
    return (send(func, _return, request) &&
            FiberManager::yield() &&
            recv());
  }

  template <class Res, class Req>
  bool fetchNoWait(void (C::*func)(google::protobuf::RpcController*,
                                   const Req*, Res*,
                                   google::protobuf::Closure*),
                   Res& _return, const Req& request) {
    if (send(func, _return, request)) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    std::shared_ptr<Protocol> protocol(new PBBinaryProtocol());
    return std::make_shared<Channel>(peer_, timeoutOpt_, protocol);
  }

private:
  std::shared_ptr<PBAsyncRpcChannel> rpcChannel_;
  std::shared_ptr<google::protobuf::RpcController> controller_;
  std::shared_ptr<C> service_;
};

}

