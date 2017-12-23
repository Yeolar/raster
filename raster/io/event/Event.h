/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>
#include <string.h>
#include <memory>
#include <arpa/inet.h>

#include "raster/coroutine/Fiber.h"
#include "raster/io/IOBuf.h"
#include "raster/io/Waker.h"
#include "raster/net/Socket.h"
#include "raster/net/Transport.h"
#include "raster/util/ContextWrapper.h"

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
    x(Waker),               \
    x(Unknown)

#define RDD_IO_EVENT_ENUM(state) k##state

class Event {
public:
  enum State {
    RDD_IO_EVENT_GEN(RDD_IO_EVENT_ENUM)
  };

  static Event* getCurrent();

  Event(const std::shared_ptr<Channel>& channel,
        const std::shared_ptr<Socket>& socket);

  Event(Waker* waker);

  virtual ~Event();

  void reset();

  Descriptor* descriptor() const { return (socket() ?: (Descriptor*)waker_); }
  int fd() const { return descriptor()->fd(); }
  Peer peer() const { return descriptor()->peer(); }

  Socket* socket() const { return socket_.get(); }

  std::string label() const;
  const char* stateName() const;

  uint64_t seqid() const { return seqid_; }

  State state() const { return state_; }
  void setState(State state) {
    state_ = state;
    record(Timestamp(state));
  }

  int group() const { return group_; }
  void setGroup(int group) { group_ = group; }

  bool isForward() const { return forward_; }
  void setForward() { forward_ = true; }

  std::shared_ptr<Channel> channel() const;
  std::unique_ptr<Processor> processor();

  Executor* executor() const { return executor_; }
  void setExecutor(Executor* executor) { executor_ = executor; }

  void restart();

  void record(Timestamp timestamp);

  uint64_t starttime() const {
    return timestamps_.empty() ? 0 : timestamps_.front().stamp;
  }
  uint64_t cost() const {
    return timestamps_.empty() ? 0 : timePassed(starttime());
  }
  std::string timestampStr() const {
    return join("-", timestamps_);
  }

  const TimeoutOption& timeoutOption() const {
    return timeoutOpt_;
  }
  Timeout<Event> edeadline() {
    return Timeout<Event>(this, starttime() + Socket::LTIMEOUT, true);
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

  template <class T, class... Args>
  void createUserContext(Args&&... args) {
    userCtx_.set(new T(std::forward<Args>(args)...));
  }
  void destroyUserContext() {
    userCtx_.dispose();
  }

  template <class T>
  T& userContext() { return *((T*)userCtx_.ptr); }
  template <class T>
  const T& userContext() const { return *((T*)userCtx_.ptr); }

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

  NOCOPY(Event);

protected:
  std::unique_ptr<Transport> transport_;

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
  std::shared_ptr<Socket> socket_;

  Waker* waker_;
  Executor* executor_;

  std::vector<Timestamp> timestamps_;
  TimeoutOption timeoutOpt_;

  ContextWrapper userCtx_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << "ev("
     << (void*)(&event) << ", "
     << *event.descriptor() << ", "
     << event.stateName() << ", "
     << event.timestampStr() << ")";
  return os;
}

#undef RDD_IO_EVENT_ENUM

} // namespace rdd
