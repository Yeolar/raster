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

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unistd.h>

#include "raster/Portability.h"
#include "raster/net/NetUtil.h"

DECLARE_uint64(net_conn_limit);
DECLARE_uint64(net_conn_timeout);

namespace raster {

#define RASTER_SOCKET_GEN(x)  \
    x(None),                      \
    x(Listener),                  \
    x(Server),                    \
    x(Client)

#define RASTER_SOCKET_ENUM(role) k##role

class Socket {
 public:
  enum Role {
    RASTER_SOCKET_GEN(RASTER_SOCKET_ENUM)
  };

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

std::ostream& operator<<(std::ostream& os, const Socket& socket);

#undef RASTER_SOCKET_ENUM

} // namespace raster
