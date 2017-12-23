/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unistd.h>

#include "raster/io/Descriptor.h"
#include "raster/net/NetUtil.h"

namespace rdd {

class Socket : public Descriptor {
public:
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

  /*
   * return:
   *  >0: read/write size
   *   0: peer is closed (make sure data exists by user)
   *  -1: error
   *  -2: again (nonblock) / timeout (block)
   *  -3: peer is closed
   */
  ssize_t recv(void* buf, size_t n);
  ssize_t send(const void* buf, size_t n);

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
  virtual Peer peer();

  bool isClient() const { return role_ == Role::kClient; }
  bool isServer() const { return role_ == Role::kServer; }

private:
  static std::atomic<size_t> count_;

  int fd_;
  Peer peer_;
};

} // namespace rdd
