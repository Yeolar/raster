/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unistd.h>
#include "rddoc/io/Descriptor.h"
#include "rddoc/net/NetUtil.h"

namespace rdd {

class Socket : public Descriptor {
public:
  enum {
    NONE,
    LISTENER,
    SERVER,
    CLIENT,
  };

  static constexpr uint64_t LTIMEOUT = 600000000; // long-polling timeout: 10min
  static constexpr size_t   LCOUNT   = 80000;     // long-polling count

  static size_t count() { return count_; }

  explicit Socket(int fd = -1);
  ~Socket();

  operator bool() const {
    return fd_ != -1;
  }

  bool bind(int port);
  bool listen(int backlog);
  std::shared_ptr<Socket> accept();

  bool connect(const std::string& host, int port);
  bool connect(const Peer& peer) {
    return connect(peer.host, peer.port);
  }
  bool isConnected();

  void close();
  bool isClosed();

  int recv(void* buf, size_t n);
  int send(void* buf, size_t n);

  bool setRecvTimeout(uint64_t t);
  bool setSendTimeout(uint64_t t);
  bool setConnTimeout(uint64_t t) {
    return setSendTimeout(t);
  }

  bool setCloseExec();
  bool setKeepAlive();
  bool setLinger(int timeout);
  bool setNonBlocking();
  bool setReuseAddr();
  bool setTCPNoDelay();

  bool getError(int& err);

  virtual int fd() const { return fd_; }
  virtual int role() const { return role_; }
  virtual char roleLabel() const { return "NLSC"[role_]; }

  Peer peer();
  virtual std::string str();

private:
  static std::atomic<size_t> count_;

  int fd_;
  int role_{NONE};
  Peer peer_;
};

} // namespace rdd
