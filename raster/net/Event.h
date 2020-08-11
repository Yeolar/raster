/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <arpa/inet.h>

#include <accelerator/AnyPtr.h>
#include <accelerator/io/IOBuf.h>

#include "raster/coroutine/Fiber.h"
#include "raster/event/EventBase.h"
#include "raster/net/Socket.h"
#include "raster/net/Transport.h"

namespace raster {

class Channel;
class Processor;

class Event : public EventBase {
 public:
  static Event* getCurrent();

  Event(std::shared_ptr<Channel> channel,
        std::unique_ptr<Socket> socket);

  ~Event();

  void reset();

  uint64_t seqid() const;

  int group() const;
  void setGroup(int group);

  bool isForward() const;
  void setForward();

  Fiber::Task* task() const;
  void setTask(Fiber::Task* task);

  // socket

  int fd() const override;
  std::string str() const override;

  Socket* socket() const;
  Peer peer() const;

  std::string label() const;

  // channel

  std::shared_ptr<Channel> channel() const;
  std::unique_ptr<Processor> processor();

  // transport

  template <class T = Transport>
  T* transport() const;

  int readData();
  int writeData();

  // callback

  void setCompleteCallback(std::function<void(Event*)> cb);
  void callbackOnComplete();

  void setCloseCallback(std::function<void(Event*)> cb);
  void callbackOnClose();

  void copyCallbacks(const Event& event);

  // user context

  template <class T, class... Args>
  void setUserContext(Args&&... args);

  template <class T>
  T& userContext();
  template <class T>
  const T& userContext() const;

  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

 private:
  static std::atomic<uint64_t> globalSeqid_;

  uint64_t seqid_;
  int group_;
  bool forward_;
  Fiber::Task* task_;

  std::shared_ptr<Channel> channel_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Transport> transport_;

  std::function<void(Event*)> completeCallback_;
  std::function<void(Event*)> closeCallback_;

  acc::UniqueAnyPtr userCtx_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << event.str();
  return os;
}

//////////////////////////////////////////////////////////////////////

inline uint64_t Event::seqid() const {
  return seqid_;
}

inline int Event::group() const {
  return group_;
}

inline void Event::setGroup(int group) {
  group_ = group;
}

inline bool Event::isForward() const {
  return forward_;
}

inline void Event::setForward() {
  forward_ = true;
}

inline Fiber::Task* Event::task() const {
  return task_;
}

inline void Event::setTask(Fiber::Task* task) {
  task_ = task;
}

inline int Event::fd() const {
  return socket_->fd();
}

inline std::string Event::str() const {
  return acc::to<std::string>(
      "ev(", socket_->str(),
      ", ", stateName(),
      ", ", timestampStr(), ")");
}

inline Socket* Event::socket() const {
  return socket_.get();
}

inline Peer Event::peer() const {
  return socket_->peer();
}

template <class T>
inline T* Event::transport() const {
  return reinterpret_cast<T*>(transport_.get());
}

inline int Event::readData() {
  return transport_->readData(socket_.get());
}
inline int Event::writeData() {
  return transport_->writeData(socket_.get());
}

template <class T, class... Args>
inline void Event::setUserContext(Args&&... args) {
  userCtx_ = acc::UniqueAnyPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
}

template <class T>
inline T& Event::userContext() {
  return *userCtx_.get<T>();
}

template <class T>
inline const T& Event::userContext() const {
  return *userCtx_.get<T>();
}

} // namespace raster
