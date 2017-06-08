/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/AsyncClient.h"
#include "rddoc/net/EventPool.h"

namespace rdd {

AsyncClient::AsyncClient(const ClientOption& option)
  : peer_(option.peer), timeout_opt_(option.timeout) {
  RDDLOG(DEBUG) << "AsyncClient: " << peer_.str()
    << ", timeout=" << timeout_opt_;
}

void AsyncClient::close() {
  freeConnection();
}

bool AsyncClient::connect() {
  if (!initConnection()) {
    return false;
  }
  if (!callback_mode_) {
    Task* task = ThreadTask::getCurrentThreadTask();
    event_->setTask(task);
    task->addBlockedCallback(
        std::bind(&Actor::addEvent, Singleton<Actor>::get(), event()));
  }
  RDDLOG(DEBUG) << "connect peer[" << peer_.str() << "]";
  return true;
}

void AsyncClient::callback() {
  RDDLOG(DEBUG) << "finish peer[" << peer_.str() << "]";
}

bool AsyncClient::initConnection() {
  if (keepalive_) {
    auto pool = Singleton<EventPoolManager>::get()->getPool(peer_.port);
    auto event = pool->get(peer_);
    if (event && event->socket()->connected()) {
      event->reset();
      event->setType(Event::CONNECT);
      event_ = event;
      return true;
    }
  }
  auto socket = std::make_shared<Socket>(0);
  if (*socket &&
      socket->setReuseAddr() &&
      (!keepalive_ || socket->setKeepAlive()) &&
      // socket->setLinger(0) &&
      socket->setTCPNoDelay() &&
      socket->setNonBlocking() &&
      socket->connect(peer_)) {
    auto event = std::make_shared<Event>(channel_, socket);
    event->setType(Event::CONNECT);
    event_ = event;
    return true;
  }
  return false;
}

void AsyncClient::freeConnection() {
  if (keepalive_) {
    auto pool = Singleton<EventPoolManager>::get()->getPool(peer_.port);
    pool->giveBack(event_);
  }
  event_ = nullptr;
}

}

