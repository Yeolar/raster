/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/AsyncClient.h"
#include "raster/io/event/EventPool.h"

namespace rdd {

AsyncClient::AsyncClient(const ClientOption& option)
  : peer_(option.peer), timeoutOpt_(option.timeout) {
  RDDLOG(DEBUG) << "AsyncClient: " << peer_.str()
    << ", timeout=" << timeoutOpt_;
}

void AsyncClient::close() {
  freeConnection();
}

bool AsyncClient::connect() {
  if (!initConnection()) {
    return false;
  }
  RDD_EVLOG(V2, event()) << "connect";
  if (!callbackMode_) {
    ExecutorPtr executor = getCurrentExecutor();
    event_->setExecutor(executor.get());
    executor->addCallback(
        std::bind(&Actor::addEvent, Singleton<Actor>::get(), event()));
  }
  return true;
}

void AsyncClient::callback() {
  RDDLOG(DEBUG) << "peer[" << peer_.str() << "] finished";
}

bool AsyncClient::initConnection() {
  if (keepalive_) {
    auto pool = Singleton<EventPoolManager>::get()->getPool(peer_.port);
    auto event = pool->get(peer_);
    if (event && event->socket()->isConnected()) {
      event->reset();
      event->setType(Event::TOWRITE);
      event_ = event;
      RDDLOG(DEBUG) << "peer[" << peer_.str() << "]"
        << " connect (keep-alive,seqid=" << event_->seqid() << ")";
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
    RDDLOG(DEBUG) << "peer[" << peer_.str() << "] connect";
    return true;
  }
  return false;
}

void AsyncClient::freeConnection() {
  if (keepalive_ && event_->type() != Event::FAIL) {
    auto pool = Singleton<EventPoolManager>::get()->getPool(peer_.port);
    pool->giveBack(event_);
  }
  event_ = nullptr;
}

} // namespace rdd
