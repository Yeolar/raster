/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>
#include <string.h>
#include <memory>
#include <arpa/inet.h>
#include "rddoc/coroutine/Fiber.h"
#include "rddoc/io/IOBuf.h"
#include "rddoc/io/Waker.h"
#include "rddoc/net/Socket.h"
#include "rddoc/util/ContextWrapper.h"
#include "rddoc/util/Function.h"

#define RDD_EVLOG(severity, ev) \
  RDDLOG(severity) << "ev(" << (void*)(ev) << ", " \
    << (ev)->roleLabel() << ":" \
    << (ev)->str() << ", " \
    << (ev)->typeName() << ") "

#define RDD_EVTLOG(severity, ev) \
  RDDLOG(severity) << "ev(" << (void*)(ev) << ", " \
    << (ev)->roleLabel() << ":" \
    << (ev)->str() << ", " \
    << (ev)->typeName() << ", " \
    << (ev)->timestampStr() << ") "

namespace rdd {

class Channel;
class Processor;

class Event : private noncopyable {
public:
  enum {
    INIT = 0, // 0
    FAIL,     // 1
    LISTEN,   // 2
    TIMEOUT,  // 3
    ERROR,    // 4
    TOREAD,   // 5
    READING,  // 6
    READED,   // 7
    TOWRITE,  // 8
    WRITING,  // 9
    WRITED,   // 10
    NEXT,     // 11
    CONNECT,  // 12
    WAKER,    // 13
  };

  enum {
    NONE = 0,
    FORWARD = 1,
  };

  static Event* getCurrentEvent();

  Event(const std::shared_ptr<Channel>& channel,
        const std::shared_ptr<Socket>& socket = std::shared_ptr<Socket>());

  Event(Waker* waker);

  virtual ~Event();

  void reset();

  Descriptor* descriptor() const { return (socket() ?: (Descriptor*)waker_); }
  int fd() const { return descriptor()->fd(); }
  int role() const { return descriptor()->role(); }
  char roleLabel() const { return descriptor()->roleLabel(); }
  std::string str() const { return descriptor()->str(); }

  Socket* socket() const { return socket_.get(); }
  Peer peer() const { return socket_->peer(); }

  std::string label() const;
  const char* typeName() const;

  int seqid() const { return seqid_; }

  int type() const { return type_; }
  void setType(int type) {
    type_ = type;
    record(Timestamp(type));
  }

  int group() const { return group_; }
  void setGroup(int group) { group_ = group; }

  bool isForward() const { return action_ == FORWARD; }
  void setForward() { action_ = FORWARD; }

  std::shared_ptr<Channel> channel() const;
  std::shared_ptr<Processor> processor(bool create = false);

  Fiber* fiber() const { return fiber_; }
  void setFiber(Fiber* fiber) { fiber_ = fiber; }

  int readData();
  int writeData();

  IOBuf* rbuf() { return rbuf_.get(); }
  IOBuf* wbuf() { return wbuf_.get(); }
  size_t& rlen() { return rlen_; }
  size_t& wlen() { return wlen_; }

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

private:
  uint64_t timeout() const {
    switch (socket_->role()) {
      case Socket::SERVER: return timeoutOpt_.wtimeout;
      case Socket::CLIENT: return timeoutOpt_.rtimeout;
    }
    return std::max(timeoutOpt_.rtimeout, timeoutOpt_.wtimeout);
  }

  int seqid_{0};
  int type_;
  int group_;
  int action_;

  std::shared_ptr<Channel> channel_;
  std::shared_ptr<Socket> socket_;
  std::shared_ptr<Processor> processor_;

  Waker* waker_;
  Fiber* fiber_;

  std::unique_ptr<IOBuf> rbuf_;
  std::unique_ptr<IOBuf> wbuf_;
  size_t rlen_; // left to read
  size_t wlen_; // already write

  std::vector<Timestamp> timestamps_;
  TimeoutOption timeoutOpt_;

  ContextWrapper userCtx_;
};

}

