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

#include "raster/net/Event.h"

#include "raster/coroutine/FiberManager.h"
#include "raster/net/Channel.h"
#include "raster/net/EventTask.h"

namespace rdd {

Event* Event::getCurrent() {
  Fiber::Task* task = getCurrentFiberTask();
  return task ?  reinterpret_cast<EventTask*>(task)->event() : nullptr;
}

std::atomic<uint64_t> Event::globalSeqid_(1);

Event::Event(std::shared_ptr<Channel> channel,
             std::unique_ptr<Socket> socket)
  : EventBase(channel->timeoutOption()),
    channel_(channel),
    socket_(std::move(socket)) {
  reset();
  ACCLOG(V2) << *this << " +";
}

Event::~Event() {
  ACCLOG(V2) << *this << " -";
}

void Event::reset() {
  restart();

  seqid_ = globalSeqid_.fetch_add(1);
  group_ = 0;
  forward_ = false;
  task_ = nullptr;

  if (transport_) {
    transport_->reset();
  } else {
    if (channel_->transportFactory()) {
      transport_ = std::move(channel_->transportFactory()->create());
      transport_->setPeerAddress(socket_->peer());
      transport_->setLocalAddress(channel_->peer());
    }
  }
}

std::string Event::label() const {
  return acc::to<std::string>(socket_->roleName()[0], channel_->id());
}

std::shared_ptr<Channel> Event::channel() const {
  return channel_;
}

std::unique_ptr<Processor> Event::processor() {
  if (!channel_->processorFactory()) {
    throw std::runtime_error("client channel has no processor");
  }
  return channel_->processorFactory()->create(this);
}

void Event::setCompleteCallback(std::function<void(Event*)> cb) {
  completeCallback_ = std::move(cb);
}
void Event::callbackOnComplete() {
  completeCallback_(this);
}

void Event::setCloseCallback(std::function<void(Event*)> cb) {
  closeCallback_ = std::move(cb);
}
void Event::callbackOnClose() {
  closeCallback_(this);
}

void Event::copyCallbacks(const Event& event) {
  completeCallback_ = event.completeCallback_;
  closeCallback_ = event.closeCallback_;
}

} // namespace rdd
