/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unistd.h>

#include "raster/net/NetUtil.h"

namespace rdd {

#define RDD_SOCKET_GEN(x)  \
    x(None),                      \
    x(Listener),                  \
    x(Server),                    \
    x(Client)

#define RDD_SOCKET_ENUM(role) k##role

class Socket {
public:
  enum Role {
    RDD_SOCKET_GEN(RDD_SOCKET_ENUM)
  };

  static constexpr uint64_t kLTimeout = 600000000;// long-polling timeout: 10min
  static constexpr size_t   kLCount   = 80000;    // long-polling count

  static size_t count() { return count_; }

  static std::unique_ptr<Socket> createSyncSocket();
  static std::unique_ptr<Socket> createAsyncSocket();

  Socket();
  Socket(int fd, const Peer& peer);

  ~Socket();

  operator bool() const {
    return fd_ != -1;
  }

  bool bind(int port);
  bool listen(int backlog);
  std::unique_ptr<Socket> accept();

  bool connect(const Peer& peer);
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

  int fd() const { return fd_; }
  const Peer& peer() const { return peer_; }

  Role role() const { return role_; }
  const char* roleName() const;

  bool isClient() const { return role_ == Role::kClient; }
  bool isServer() const { return role_ == Role::kServer; }

private:
  static std::atomic<size_t> count_;

  int fd_{-1};
  Peer peer_;
  Role role_{kNone};
};

inline std::ostream& operator<<(std::ostream& os, const Socket& socket) {
  os << socket.roleName()[0] << ":" << socket.fd()
     << "[" << socket.peer() << "]";
  return os;
}

#undef RDD_SOCKET_ENUM

} // namespace rdd
