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

#include "raster/net/Acceptor.h"

namespace rdd {

Acceptor::Acceptor(std::shared_ptr<NetHub> hub)
  : hub_(hub), loop_(acc::make_unique<acc::EventLoop>()) {
}

void Acceptor::addService(std::unique_ptr<Service> service) {
  services_.emplace(service->name(), std::move(service));
}

void Acceptor::configService(
    const std::string& name, int port, const TimeoutOption& timeout) {
  auto service = acc::get_deref_smart_ptr(services_, name);
  if (!service) {
    ACCLOG(FATAL) << "service: [" << name << "] not added";
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
  ACCLOG(INFO) << *event << " listen on port=" << port;
}

void Acceptor::start() {
  loop_->loop();
}

void Acceptor::stop() {
  loop_->stop();
}

} // namespace rdd
