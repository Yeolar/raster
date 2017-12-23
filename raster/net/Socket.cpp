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
#include "raster/util/Logging.h"

namespace rdd {

std::atomic<size_t> Socket::count_(0);

Socket::Socket(int fd) : Descriptor(Role::kNone), fd_(fd) {
  if (fd_ == 0) {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
  }
  if (fd_ == -1) {
    RDDPLOG(ERROR) << "create socket failed";
  } else {
    ++count_;
    role_ = Role::kServer;
  }
}

Socket::~Socket() {
  if (fd_ != -1) {
    close();
  }
}

bool Socket::bind(int port) {
  peer_.port = port;
  role_ = Role::kListener;

  struct sockaddr_in sin;
  bzero(&sin, sizeof(sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  socklen_t len = sizeof(sin);

  int r = ::bind(fd_, (struct sockaddr *)&sin, len);
  if (r == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): bind failed on port=" << peer_.port;
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

std::shared_ptr<Socket> Socket::accept() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  int fd = ::accept(fd_, (struct sockaddr *)&sin, &len);
  if (fd == -1) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): accept error";
  }
  return std::make_shared<Socket>(fd);
}

bool Socket::connect(const std::string& host, int port) {
  peer_ = {host, port};
  role_ = Role::kClient;

  struct addrinfo *ai, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  auto service = to<std::string>(port);
  int r = getaddrinfo(host.c_str(), service.c_str(), &hints, &ai);
  if (r != 0) {
    RDDLOG(ERROR) << gai_strerror(r) << " fd(" << fd_ << "): "
      << "getaddrinfo failed on host=" << host << ", port=" << port;
    return false;
  }
  if (ai == nullptr) {
    RDDLOG(ERROR) << "fd(" << fd_ << "): "
      << "no addinfo found on host=" << host << ", port=" << port;
    return false;
  }
  r = ::connect(fd_, (struct sockaddr*)ai->ai_addr, ai->ai_addrlen);
  if (r == -1 && errno != EINPROGRESS && errno != EWOULDBLOCK) {
    RDDPLOG(ERROR) << "fd(" << fd_ << "): "
      << "connect failed on host=" << host << ", port=" << port;
    freeaddrinfo(ai);
    return false;
  }
  freeaddrinfo(ai);
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

Peer Socket::peer() {
  if (peer_.port == 0 && role_ == Role::kServer) {
    peer_ = Peer(fd_);
  }
  return peer_;
}

} // namespace rdd
