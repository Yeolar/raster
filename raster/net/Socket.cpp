/*
 * Copyright (C) 2017, Yeolar
 */

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "raster/io/FileUtil.h"
#include "raster/net/Socket.h"
#include "raster/util/Conv.h"
#include "raster/util/Memory.h"
#include "raster/util/Logging.h"
#include "raster/util/Time.h"

namespace rdd {

std::atomic<size_t> Socket::count_(0);

std::unique_ptr<Socket> Socket::createSyncSocket() {
  auto socket = make_unique<Socket>();
  if (*socket) {
    socket->setReuseAddr();
    socket->setTCPNoDelay();
    return socket;
  }
  return nullptr;
}

std::unique_ptr<Socket> Socket::createAsyncSocket() {
  auto socket = make_unique<Socket>();
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
    RDDPLOG(ERROR) << "fd(" << fd_ << "): bind failed on port=" << peer_.port();
  }
  return r != -1;
}

bool Socket::listen(int backlog) {
  int r = ::listen(fd_, backlog);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): listen failed";
  }
  return r != -1;
}

std::unique_ptr<Socket> Socket::accept() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  int fd = ::accept(fd_, (struct sockaddr*)&sin, &len);
  if (fd == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): accept error";
    return nullptr;
  }
  Peer peer;
  peer.setFromSockaddr((struct sockaddr*)&sin);
  return make_unique<Socket>(fd, peer);
}

bool Socket::connect(const Peer& peer) {
  role_ = Role::kClient;
  peer_ = peer;

  sockaddr_storage tmp_sock;
  socklen_t len = peer_.getAddress(&tmp_sock);
  int r = ::connect(fd_, (struct sockaddr*)&tmp_sock, len);
  if (r == -1 && errno != EINPROGRESS && errno != EWOULDBLOCK) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): "
      << "connect failed on peer=" << peer_;
    return false;
  }
  return true;
}

bool Socket::isConnected() {
  struct tcp_info info;
  socklen_t len = sizeof(info);
  int r = getsockopt(fd_, IPPROTO_TCP, TCP_INFO, &info, &len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): get TCP_INFO failed";
    return false;
  }
  return info.tcpi_state == TCP_ESTABLISHED;
}

void Socket::close() {
  if (closeNoInt(fd_) != 0) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): close failed";
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
  // Note the use of MSG_NOSIGNAL to suppress SIGPIPE errors, instead we
  // check for the EPIPE return condition and close the socket in that case
  while (true) {
    ssize_t r = ::send(fd_, buf, n, MSG_NOSIGNAL);
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
  struct timeval tv = toTimeval(t);
  socklen_t len = sizeof(tv);
  int r = setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set SO_RCVTIMEO failed";
  }
  return r != -1;
}

bool Socket::setSendTimeout(uint64_t t) {
  struct timeval tv = toTimeval(t);
  socklen_t len = sizeof(tv);
  int r = setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set SO_SNDTIMEO failed";
  }
  return r != -1;
}

bool Socket::setCloseExec() {
  if (!rdd::setCloseExec(fd_)) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set FD_CLOEXEC failed";
    return false;
  }
  return true;
}

bool Socket::setKeepAlive() {
  int keepalive = 1;
  int keepidle = 600;
  int keepinterval = 5;
  int keepcount = 2;
  socklen_t len = sizeof(int);
  int r = setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &keepalive, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set SO_KEEPALIVE failed";
    return false;
  }
  setsockopt(fd_, SOL_TCP, TCP_KEEPIDLE, &keepidle, len);
  setsockopt(fd_, SOL_TCP, TCP_KEEPINTVL, &keepinterval, len);
  setsockopt(fd_, SOL_TCP, TCP_KEEPCNT, &keepcount, len);
  return r != -1;
}

bool Socket::setLinger(int timeout) {
  struct linger linger = {1, timeout};
  socklen_t len = sizeof(linger);
  int r = setsockopt(fd_, SOL_SOCKET, SO_LINGER, &linger, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set SO_LINGER failed";
  }
  return r != -1;
}

bool Socket::setNonBlocking() {
  if (!rdd::setNonBlocking(fd_)) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set O_NONBLOCK failed";
    return false;
  }
  return true;
}

bool Socket::setReuseAddr() {
  int reuse = 1;
  socklen_t len = sizeof(reuse);
  int r = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set SO_REUSEADDR failed";
  }
  return r != -1;
}

bool Socket::setTCPNoDelay() {
  int nodelay = 1;
  socklen_t len = sizeof(nodelay);
  int r = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): set TCP_NODELAY failed";
  }
  return r != -1;
}

bool Socket::getError(int& err) {
  socklen_t len = sizeof(err);
  int r = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): get SO_ERROR failed";
  }
  return r != -1;
}

} // namespace rdd
