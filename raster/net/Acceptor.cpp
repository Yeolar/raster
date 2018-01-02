/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/Acceptor.h"

namespace rdd {

Acceptor::Acceptor(std::shared_ptr<NetHub> hub)
  : hub_(hub), loop_(make_unique<EventLoop>()) {
}

void Acceptor::addService(std::unique_ptr<Service> service) {
  services_.emplace(service->name(), std::move(service));
}

void Acceptor::configService(
    const std::string& name, int port, const TimeoutOption& timeout) {
  auto service = get_deref_smart_ptr(services_, name);
  if (!service) {
    RDDLOG(FATAL) << "service: [" << name << "] not added";
    return;
  }

  service->makeChannel(port, timeout);
  listen(service);
}

void Acceptor::listen(Service* service, int backlog) {
  int port = service->channel()->id();
  auto socket = Socket::createAsyncSocket();
  if (!socket ||
      !(socket->bind(port)) ||
      !(socket->listen(backlog))) {
    throw std::runtime_error("socket listen failed");
  }

  auto event = new Event(service->channel(), std::move(socket));
  event->setState(Event::kListen);
  event->setCompleteCallback([&](Event* ev) { hub_->execute(ev); });
  event->setCloseCallback([&](Event* ev) { hub_->execute(ev); });
  loop_->addEvent(event);
  RDDLOG(INFO) << *event << " listen on port=" << port;
}

void Acceptor::start() {
  loop_->loop();
}

void Acceptor::stop() {
  loop_->stop();
}

} // namespace rdd
