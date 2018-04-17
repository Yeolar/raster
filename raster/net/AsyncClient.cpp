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

#include "raster/net/AsyncClient.h"

#include "raster/net/EventPool.h"

namespace rdd {

AsyncClient::AsyncClient(std::shared_ptr<NetHub> hub,
                         const Peer& peer,
                         const TimeoutOption& timeout)
  : hub_(hub), peer_(peer), timeout_(timeout) {
  ACCLOG(DEBUG) << "AsyncClient: " << peer_ << ", timeout=" << timeout_;
}

AsyncClient::AsyncClient(std::shared_ptr<NetHub> hub,
                         const Peer& peer,
                         uint64_t ctimeout,
                         uint64_t rtimeout,
                         uint64_t wtimeout)
  : AsyncClient(hub, peer, {ctimeout, rtimeout, wtimeout}) {}

AsyncClient::AsyncClient(std::shared_ptr<NetHub> hub,
                         const ClientOption& option)
  : AsyncClient(hub, option.peer, option.timeout) {}

void AsyncClient::close() {
  freeConnection();
}

bool AsyncClient::connect() {
  if (!initConnection()) {
    return false;
  }
  ACCLOG(V2) << *event() << " connect";
  Fiber::Task* task = getCurrentFiberTask();
  event_->setTask(task);
  task->blockCallbacks.push_back([&]() { hub_->addEvent(event()); });
  return true;
}

bool AsyncClient::initConnection() {
  if (keepalive_) {
    auto pool = acc::Singleton<EventPoolManager>::get()->getPool(peer_.port());
    auto event = pool->get(peer_);
    if (event && event->socket()->isConnected()) {
      event->reset();
      event->setState(Event::kToWrite);
      event_ = std::move(event);
      ACCLOG(DEBUG) << "peer[" << peer_ << "]"
        << " connect (keep-alive,seqid=" << event_->seqid() << ")";
      return true;
    }
  }
  auto socket = Socket::createAsyncSocket();
  if (socket &&
      (!keepalive_ || socket->setKeepAlive()) &&
      socket->connect(peer_)) {
    auto event = acc::make_unique<Event>(channel_, std::move(socket));
    event->setState(Event::kConnect);
    event_ = std::move(event);
    ACCLOG(DEBUG) << "peer[" << peer_ << "] connect";
    return true;
  }
  return false;
}

void AsyncClient::freeConnection() {
  if (keepalive_ && event_->state() != Event::kFail) {
    auto pool = acc::Singleton<EventPoolManager>::get()->getPool(peer_.port());
    pool->giveBack(std::move(event_));
  }
  event_ = nullptr;
}

bool yieldMultiTask(std::initializer_list<AsyncClient*> clients) {
  int size = clients.size();
  if (size != 0) {
    std::vector<Event*> events(size);
    std::vector<NetHub*> hubs(size);
    for (auto& i : clients) {
      if (i->connected()) {
        events.push_back(i->event());
        hubs.push_back(i->hub());
      }
    }
    size = hubs.size();
    if (size > 0) {
      if (std::count(hubs.begin(), hubs.end(), hubs[0]) == size) {
        if (hubs[0]->waitGroup(events)) {
          return FiberManager::yield();
        }
      }
    }
  }
  return false;
}

} // namespace rdd
