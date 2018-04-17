/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <initializer_list>

#include "raster/coroutine/FiberManager.h"
#include "raster/net/Event.h"
#include "raster/net/NetHub.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"

/*
 * AsyncClient and its inherit classes have 2 ways of usage:
 *  1. single async client
 *  2. multiple async client
 */
namespace rdd {

class AsyncClient {
 public:
  AsyncClient(std::shared_ptr<NetHub> hub,
              const ClientOption& option);

  AsyncClient(std::shared_ptr<NetHub> hub,
              const Peer& peer,
              const TimeoutOption& timeout);

  AsyncClient(std::shared_ptr<NetHub> hub,
              const Peer& peer,
              uint64_t ctimeout = 100000,
              uint64_t rtimeout = 1000000,
              uint64_t wtimeout = 300000);

  virtual ~AsyncClient() {
    close();
  }

  virtual bool connect();

  virtual bool connected() const {
    return event_ != nullptr;
  }

  virtual void close();

  NetHub* hub() const {
    return hub_.get();
  }

  Event* event() const {
    return event_.get();
  }

  void setKeepAlive() {
    keepalive_ = true;
  }

  bool keepAlive() const {
    return keepalive_;
  }

 protected:
  virtual std::shared_ptr<Channel> makeChannel() = 0;

  bool initConnection();
  void freeConnection();

  std::shared_ptr<NetHub> hub_;
  Peer peer_;
  TimeoutOption timeout_;
  bool keepalive_{false};
  std::unique_ptr<Event> event_;
  std::shared_ptr<Channel> channel_;
};

template <class C>
class MultiAsyncClient {
 public:
  MultiAsyncClient(std::shared_ptr<NetHub> hub,
                   size_t count,
                   const ClientOption& option)
    : hub_(hub) {
    for (size_t i = 0; i < count; ++i) {
      clients_.push_back(acc::make_unique<C>(hub_, option));
    }
  }
  MultiAsyncClient(std::shared_ptr<NetHub> hub,
                   const std::vector<ClientOption>& options)
    : hub_(hub) {
    for (auto& i : options) {
      clients_.push_back(acc::make_unique<C>(hub_, i));
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
      if (!hub_->waitGroup(events)) {
        return false;
      }
      return FiberManager::yield();
    }
    return false;
  }

  C* operator[](size_t i) { return clients_[i].get(); }

 private:
  std::shared_ptr<NetHub> hub_;
  std::vector<std::unique_ptr<C>> clients_;
};

// yield task for multiple clients with different types
bool yieldMultiTask(std::initializer_list<AsyncClient*> clients);

} // namespace rdd
