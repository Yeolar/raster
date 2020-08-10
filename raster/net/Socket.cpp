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

#include "raster/net/Socket.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <accelerator/Conv.h>
#include <accelerator/FileUtil.h>
#include <accelerator/Logging.h>
#include <accelerator/Memory.h>
#include <accelerator/Time.h>

DEFINE_uint64(net_conn_limit, 100000,
              "Limit # of net connection.");
DEFINE_uint64(net_conn_timeout, 600000000,
              "Long-polling timeout # of net connection.");

#define RASTER_SOCKET_STR(role) #role

namespace {
  static const char* roleStrings[] = {
    RASTER_SOCKET_GEN(RASTER_SOCKET_STR)
  };
}

namespace raster {

std::atomic<size_t> Socket::count_(0);

std::unique_ptr<Socket> Socket::createSyncSocket() {
  auto socket = std::make_unique<Socket>();
  if (*socket) {
    socket->setReuseAddr();
    socket->setTCPNoDelay();
    return socket;
  }
  return nullptr;
}

std::unique_ptr<Socket> Socket::createAsyncSocket() {
  auto socket = std::make_unique<Socket>();
  if (*socket) {
    socket->setReuseAddr();
    //socket->setLinger(0);
    socket->setTCPNoDelay();
    socket->setNonBlocking();
    return socket;
  }
  return nullptr;
}

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  ++count_;
}

Socket::Socket(int fd, const Peer& peer)
  : fd_(fd), peer_(peer) {
  role_ = kServer;
  ++count_;
}

Socket::~Socket() {
  if (fd_ != -1) {
    close();
  }
}

bool Socket::bind(int port) {
  role_ = Role::kListener;
  peer_.setFromLocalPort(port);

  sockaddr_storage tmp_sock;
  socklen_t len = peer_.getAddress(&tmp_sock);
  int r = ::bind(fd_, (struct sockaddr*)&tmp_sock, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): bind failed on port=" << peer_.port();
  }
  return r != -1;
}

bool Socket::listen(int backlog) {
  int r = ::listen(fd_, backlog);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): listen failed";
  }
  return r != -1;
}

std::unique_ptr<Socket> Socket::accept() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  int fd = ::accept(fd_, (struct sockaddr*)&sin, &len);
  if (fd == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): accept error";
    return nullptr;
  }
  Peer peer;
  peer.setFromSockaddr((struct sockaddr*)&sin);
  return std::make_unique<Socket>(fd, peer);
}

bool Socket::connect(const Peer& peer) {
  role_ = Role::kClient;
  peer_ = peer;

  sockaddr_storage tmp_sock;
  socklen_t len = peer_.getAddress(&tmp_sock);
  int r = ::connect(fd_, (struct sockaddr*)&tmp_sock, len);
  if (r == -1 && errno != EINPROGRESS && errno != EWOULDBLOCK) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): "
      << "connect failed on peer=" << peer_;
    return false;
  }
  return true;
}

bool Socket::isConnected() {
#if __linux__
  struct tcp_info info;
  socklen_t len = sizeof(info);
  int r = getsockopt(fd_, IPPROTO_TCP, TCP_INFO, &info, &len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): get TCP_INFO failed";
    return false;
  }
  return info.tcpi_state == TCP_ESTABLISHED;
#else
  int val;
  socklen_t len = sizeof(val);
  int r = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &val, &len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): get SO_ERROR failed";
    return false;
  }
  return val == 0;
#endif
}

void Socket::close() {
  if (acc::closeNoInt(fd_) != 0) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): close failed";
  }
  fd_ = -1;
  --count_;
}

bool Socket::isClosed() {
  char p[8];
  return recv(p, sizeof(p)) == 0;
}

ssize_t Socket::recv(void* buf, size_t n) {
  while (true) {
    ssize_t r = ::recv(fd_, buf, n, 0);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        return -2;
      }
      if (errno == ECONNRESET) {
        return -3;
      }
    }
    return r;
  }
}

ssize_t Socket::send(const void* buf, size_t n) {
  int flags = 0;
#ifdef MSG_NOSIGNAL
  // Note the use of MSG_NOSIGNAL to suppress SIGPIPE errors, instead we
  // check for the EPIPE return condition and close the socket in that case
  flags |= MSG_NOSIGNAL;
#endif
  while (true) {
    ssize_t r = ::send(fd_, buf, n, flags);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        return -2;
      }
    }
    return r;
  }
}

bool Socket::setRecvTimeout(uint64_t t) {
  struct timeval tv = acc::toTimeval(t);
  socklen_t len = sizeof(tv);
  int r = setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set SO_RCVTIMEO failed";
  }
  return r != -1;
}

bool Socket::setSendTimeout(uint64_t t) {
  struct timeval tv = acc::toTimeval(t);
  socklen_t len = sizeof(tv);
  int r = setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set SO_SNDTIMEO failed";
  }
  return r != -1;
}

bool Socket::setCloseExec() {
  if (!acc::setCloseExec(fd_)) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set FD_CLOEXEC failed";
    return false;
  }
  return true;
}

bool Socket::setKeepAlive() {
  int keepalive = 1;
  socklen_t len = sizeof(int);
  int r = setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &keepalive, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set SO_KEEPALIVE failed";
    return false;
  }
#if __linux__
  int keepidle = 600;
  int keepinterval = 5;
  int keepcount = 2;
  setsockopt(fd_, SOL_TCP, TCP_KEEPIDLE, &keepidle, len);
  setsockopt(fd_, SOL_TCP, TCP_KEEPINTVL, &keepinterval, len);
  setsockopt(fd_, SOL_TCP, TCP_KEEPCNT, &keepcount, len);
#endif
  return r != -1;
}

bool Socket::setLinger(int timeout) {
  struct linger linger = {1, timeout};
  socklen_t len = sizeof(linger);
  int r = setsockopt(fd_, SOL_SOCKET, SO_LINGER, &linger, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set SO_LINGER failed";
  }
  return r != -1;
}

bool Socket::setNonBlocking() {
  if (!acc::setNonBlocking(fd_)) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set O_NONBLOCK failed";
    return false;
  }
  return true;
}

bool Socket::setReuseAddr() {
  int reuse = 1;
  socklen_t len = sizeof(reuse);
  int r = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set SO_REUSEADDR failed";
  }
  return r != -1;
}

bool Socket::setTCPNoDelay() {
  int nodelay = 1;
  socklen_t len = sizeof(nodelay);
  int r = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): set TCP_NODELAY failed";
  }
  return r != -1;
}

bool Socket::getError(int& err) {
  socklen_t len = sizeof(err);
  int r = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
  if (r == -1) {
    ACCPLOG(ERROR) << "fd(" << fd_ << "): get SO_ERROR failed";
  }
  return r != -1;
}

const char* Socket::roleName() const {
  return roleStrings[role_];
}

std::string Socket::str() const {
  return acc::to<std::string>(
      roleName()[0], ":", fd_, "[", peer_.describe(), "]");
}

} // namespace raster
