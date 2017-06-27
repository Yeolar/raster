/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <initializer_list>
#include "rddoc/coroutine/Fiber.h"
#include "rddoc/io/event/Event.h"
#include "rddoc/net/Actor.h"
#include "rddoc/net/NetUtil.h"
#include "rddoc/net/Socket.h"

/*
 * AsyncClient and its inherit classes have 3 ways of usage:
 *  1. single async client
 *  2. multiple async client
 *  3. single async client by callback mode
 */
namespace rdd {

class AsyncClient {
public:
  AsyncClient(const ClientOption& option);

  virtual ~AsyncClient() {
    close();
  }

  void setKeepAlive() { keepalive_ = true; }
  bool keepAlive() const { return keepalive_; }

  // if use callback mode, you should new the client
  void setCallbackMode() { callbackMode_ = true; }
  bool callbackMode() const { return callbackMode_; }

  virtual bool connect();
  virtual void callback();
  virtual void close();

  virtual bool connected() const {
    return event_ != nullptr;
  }

  Event* event() const { return event_.get(); }

protected:
  virtual std::shared_ptr<Channel> makeChannel() = 0;

  bool initConnection();
  void freeConnection();

  Peer peer_;
  TimeoutOption timeoutOpt_;
  bool keepalive_{false};
  bool callbackMode_{false};
  std::shared_ptr<Event> event_;
  std::shared_ptr<Channel> channel_;
};

template <class C>
class MultiAsyncClient {
public:
  MultiAsyncClient(size_t count,
                   const std::string& host,
                   int port,
                   uint64_t ctimeout = 100000,
                   uint64_t rtimeout = 1000000,
                   uint64_t wtimeout = 300000) {
    for (size_t i = 0; i < count; ++i) {
      clients_.push_back(
          std::make_shared<C>(
              host, port, ctimeout, rtimeout, wtimeout));
    }
  }
  MultiAsyncClient(size_t count, const ClientOption& option) {
    for (size_t i = 0; i < count; ++i) {
      clients_.push_back(std::make_shared<C>(option));
    }
  }
  MultiAsyncClient(const std::vector<ClientOption>& options) {
    for (auto& i : options) {
      clients_.push_back(std::make_shared<C>(i));
    }
  }

  size_t count() const { return clients_.size(); }

  bool connect() {
    for (auto& i : clients_) {
      if (!i->connect()) return false;
    }
    return true;
  }
  bool connect(size_t i) {
    assert(i < count());
    return clients_[i]->connect();
  }

  bool connected() const {
    for (auto& i : clients_) {
      if (!i->connected()) return false;
    }
    return true;
  }
  bool connected(size_t i) {
    assert(i < count());
    return clients_[i]->connected();
  }

  template <class Res>
  bool recv(size_t i, Res& response) {
    assert(i < count());
    return clients_[i]->recv(response);
  }

  template <class T, class Res>
  bool recv(size_t i, void (T::*recvFunc)(Res&), Res& _return) {
    assert(i < count());
    return clients_[i]->recv(recvFunc, _return);
  }

  template <class T, class Res>
  bool recv(size_t i, Res (T::*recvFunc)(void), Res& _return) {
    assert(i < count());
    return clients_[i]->recv(recvFunc, _return);
  }

  template <class Req>
  bool send(size_t i, Req& request) {
    assert(i < count());
    return clients_[i]->send(request);
  }

  template <class T, class... Req>
  bool send(size_t i, void (T::*sendFunc)(const Req&...),
            const Req&... requests) {
    assert(i < count());
    return clients_[i]->send(sendFunc, requests...);
  }

  bool yield() {
    if (!clients_.empty()) {
      std::vector<Event*> events;
      for (auto& i : clients_) {
        if (i->connected()) {
          events.push_back(i->event());
        }
      }
      if (!Singleton<Actor>::get()->waitGroup(events)) {
        return false;
      }
      return FiberManager::yield();
    }
    return false;
  }

  std::shared_ptr<C> operator[](size_t i) { return clients_[i]; }

private:
  std::vector<std::shared_ptr<C>> clients_;
};

// yield task for multiple clients with different types
//
inline bool yieldMultiTask(std::initializer_list<AsyncClient*> clients) {
  if (clients.size() != 0) {
    std::vector<Event*> events;
    for (auto& i : clients) {
      if (i->connected()) {
        events.push_back(i->event());
      }
    }
    if (!Singleton<Actor>::get()->waitGroup(events)) {
      return false;
    }
    return FiberManager::yield();
  }
  return false;
}

}

