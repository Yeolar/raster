/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <arpa/inet.h>

#include "raster/net/AsyncClient.h"
#include "raster/protocol/http/Protocol.h"
#include "raster/util/Logging.h"
#include "raster/util/ScopeGuard.h"

/*
 * callback mode:
 * (if you have a thrift TClient, and want to do some custom work)
 *
 *  class MyAsyncClient : public HTTPAsyncClient {
 *  public:
 *    MyAsyncClient(const ClientOption& option)
 *      : HTTPAsyncClient<TClient>(option, true) {}  // use callback mode
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

class HTTPAsyncClient : public AsyncClient {
public:
  HTTPAsyncClient(const ClientOption& option)
    : AsyncClient(option) {
    channel_ = makeChannel();
  }
  HTTPAsyncClient(const std::string& host,
               int port,
               uint64_t ctimeout = 100000,
               uint64_t rtimeout = 1000000,
               uint64_t wtimeout = 300000)
    : HTTPAsyncClient({Peer(host, port), {ctimeout, rtimeout, wtimeout}}) {
  }
  virtual ~HTTPAsyncClient() {}

  bool recv() {
    if (!event_ || event_->type() == Event::FAIL) {
      return false;
    }
    return true;
  }

  bool send(XXX) {
    if (!event_) {
      return false;
    }
    return true;
  }

  bool fetch(XXX) {
    return (send(XXX) &&
            FiberManager::yield() &&
            recv());
  }

  bool fetchNoWait(XXX) {
    if (send(XXX)) {
      Singleton<Actor>::get()->execute((AsyncClient*)this);
      return true;
    }
    return false;
  }

protected:
  virtual std::shared_ptr<Channel> makeChannel() {
    std::shared_ptr<Protocol> protocol(new HTTPProtocol());
    return std::make_shared<Channel>(
        Channel::HTTP, peer_, timeoutOpt_, protocol);
  }
};

} // namespace rdd
