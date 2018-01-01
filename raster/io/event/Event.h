/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>
#include <string.h>
#include <iostream>
#include <memory>
#include <arpa/inet.h>

#include "raster/coroutine/Fiber.h"
#include "raster/io/IOBuf.h"
#include "raster/net/Socket.h"
#include "raster/net/Transport.h"
#include "raster/util/DynamicPtr.h"

namespace rdd {

class Channel;
class Processor;

#define RDD_IO_EVENT_GEN(x) \
    x(Init),                \
    x(Connect),             \
    x(Listen),              \
    x(ToRead),              \
    x(Reading),             \
    x(Readed),              \
    x(ToWrite),             \
    x(Writing),             \
    x(Writed),              \
    x(Next),                \
    x(Fail),                \
    x(Timeout),             \
    x(Error),               \
    x(Unknown)

#define RDD_IO_EVENT_ENUM(state) k##state

class Event {
 public:
  enum State {
    RDD_IO_EVENT_GEN(RDD_IO_EVENT_ENUM)
  };

  static Event* getCurrent();

  Event(std::shared_ptr<Channel> channel,
        std::unique_ptr<Socket> socket);

  ~Event();

  void reset();

  uint64_t seqid() const { return seqid_; }

  State state() const { return state_; }
  void setState(State state);
  const char* stateName() const;

  int group() const { return group_; }
  void setGroup(int group) { group_ = group; }

  bool isForward() const { return forward_; }
  void setForward() { forward_ = true; }

  Fiber::Task* task() const { return task_; }
  void setTask(Fiber::Task* task) { task_ = task; }

  // time

  void restart();

  uint64_t starttime() const {
    return timestamps_.front().stamp;
  }
  uint64_t cost() const {
    return timePassed(starttime());
  }
  std::string timestampStr() const {
    return join("-", timestamps_);
  }

  // timeout

  const TimeoutOption& timeoutOption() const {
    return timeoutOpt_;
  }
  Timeout<Event> edeadline() {
    return Timeout<Event>(this, starttime() + FLAGS_net_conn_timeout, true);
  }
  Timeout<Event> cdeadline() {
    return Timeout<Event>(this, starttime() + timeoutOpt_.ctimeout);
  }
  Timeout<Event> rdeadline() {
    return Timeout<Event>(this, starttime() + timeoutOpt_.rtimeout);
  }
  Timeout<Event> wdeadline() {
    return Timeout<Event>(this, starttime() + timeoutOpt_.wtimeout);
  }

  bool isConnectTimeout() const {
    return cost() > timeoutOpt_.ctimeout;
  }
  bool isReadTimeout() const {
    return cost() > timeoutOpt_.rtimeout;
  }
  bool isWriteTimeout() const {
    return cost() > timeoutOpt_.wtimeout;
  }

  // socket

  Socket* socket() const { return socket_.get(); }
  int fd() const { return socket_->fd(); }
  Peer peer() const { return socket_->peer(); }

  std::string label() const;

  // channel

  std::shared_ptr<Channel> channel() const;
  std::unique_ptr<Processor> processor();

  // transport

  template <class T = Transport>
  T* transport() const {
    return reinterpret_cast<T*>(transport_.get());
  }

  int readData() {
    return transport_->readData(socket_.get());
  }
  int writeData() {
    return transport_->writeData(socket_.get());
  }

  // callback

  void setCompleteCallback(std::function<void(Event*)> cb);
  void callbackOnComplete();

  void setCloseCallback(std::function<void(Event*)> cb);
  void callbackOnClose();

  void copyCallbacks(const Event& event);

  // user context

  template <class T, class... Args>
  void setUserContext(Args&&... args) {
    userCtx_.set(new T(std::forward<Args>(args)...));
  }

  template <class T>
  T& userContext() { return *userCtx_.get<T>(); }
  template <class T>
  const T& userContext() const { return *userCtx_.get<T>(); }

  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

 private:
  uint64_t timeout() const {
    if (socket_->isClient()) return timeoutOpt_.rtimeout;
    if (socket_->isServer()) return timeoutOpt_.wtimeout;
    return std::max(timeoutOpt_.rtimeout, timeoutOpt_.wtimeout);
  }

  static std::atomic<uint64_t> globalSeqid_;

  uint64_t seqid_;
  State state_;
  int group_;
  bool forward_;

  std::shared_ptr<Channel> channel_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Transport> transport_;

  Fiber::Task* task_;

  std::vector<Timestamp> timestamps_;
  TimeoutOption timeoutOpt_;

  std::function<void(Event*)> completeCallback_;
  std::function<void(Event*)> closeCallback_;

  DynamicPtr userCtx_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << "ev(" << *event.socket()
     << ", " << event.stateName()
     << ", " << event.timestampStr() << ")";
  return os;
}

#undef RDD_IO_EVENT_ENUM

} // namespace rdd
