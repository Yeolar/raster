/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/AsyncClient.h"

namespace rdd {

AsyncClient::AsyncClient(const ClientOption& option, bool on_finish)
  : peer_(option.peer)
  , timeout_opt_(option.timeout)
  , on_finish_(on_finish) {
  RDDLOG(DEBUG) << "AsyncClient: " << peer_.str()
    << ", timeout=" << timeout_opt_;
}

void AsyncClient::close() {
  if (event_) {
    delete event_;
  }
  event_ = nullptr;
}

bool AsyncClient::connect() {
  auto socket = std::make_shared<Socket>(0);
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking()) ||
      !(socket->connect(peer_))) {
    return false;
  }
  Event *event = new Event(channel_, socket);
  if (!event) {
    RDDLOG(ERROR) << "create event failed";
    return false;
  }
  event->setType(Event::CONNECT);
  event_ = event;
  if (!on_finish_) {
    Task* task = ThreadTask::getCurrentThreadTask();
    event->setTask(task);
    task->addBlockedCallback(
        std::bind(&Actor::addEvent, Singleton<Actor>::get(), event));
  }
  RDDLOG(DEBUG) << "connect peer[" << peer_.str() << "]";
  return true;
}

void AsyncClient::onFinish() {
  if (on_finish_) {  // always true
    // ...
    RDDLOG(DEBUG) << "finish peer[" << peer_.str() << "]";
    delete this;
  }
}

}

