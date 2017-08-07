/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <cstring>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include "raster/util/Logging.h"

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

template <class T>
inline const void* const_cast_sockopt(const T* v) {
  return reinterpret_cast<const void*>(v);
}

template <class T>
inline void* cast_sockopt(T* v) {
  return reinterpret_cast<void*>(v);
}

void destroyer_of_fine_sockets(int* ssock) {
  ::close(*ssock);
  delete ssock;
}

namespace apache {
namespace thrift {
namespace transport {

using namespace std;
using boost::shared_ptr;

TServerSocket::TServerSocket(int port)
  : port_(port),
    serverSocket_(-1),
    acceptBacklog_(DEFAULT_BACKLOG),
    sendTimeout_(0),
    recvTimeout_(0),
    accTimeout_(-1),
    retryLimit_(0),
    retryDelay_(0),
    tcpSendBuffer_(0),
    tcpRecvBuffer_(0),
    keepAlive_(false),
    interruptableChildren_(true),
    listening_(false),
    interruptSockWriter_(-1),
    interruptSockReader_(-1),
    childInterruptSockWriter_(-1) {
}

TServerSocket::TServerSocket(int port, int sendTimeout, int recvTimeout)
  : port_(port),
    serverSocket_(-1),
    acceptBacklog_(DEFAULT_BACKLOG),
    sendTimeout_(sendTimeout),
    recvTimeout_(recvTimeout),
    accTimeout_(-1),
    retryLimit_(0),
    retryDelay_(0),
    tcpSendBuffer_(0),
    tcpRecvBuffer_(0),
    keepAlive_(false),
    interruptableChildren_(true),
    listening_(false),
    interruptSockWriter_(-1),
    interruptSockReader_(-1),
    childInterruptSockWriter_(-1) {
}

TServerSocket::TServerSocket(const string& address, int port)
  : port_(port),
    address_(address),
    serverSocket_(-1),
    acceptBacklog_(DEFAULT_BACKLOG),
    sendTimeout_(0),
    recvTimeout_(0),
    accTimeout_(-1),
    retryLimit_(0),
    retryDelay_(0),
    tcpSendBuffer_(0),
    tcpRecvBuffer_(0),
    keepAlive_(false),
    interruptableChildren_(true),
    listening_(false),
    interruptSockWriter_(-1),
    interruptSockReader_(-1),
    childInterruptSockWriter_(-1) {
}

TServerSocket::TServerSocket(const string& path)
  : port_(0),
    path_(path),
    serverSocket_(-1),
    acceptBacklog_(DEFAULT_BACKLOG),
    sendTimeout_(0),
    recvTimeout_(0),
    accTimeout_(-1),
    retryLimit_(0),
    retryDelay_(0),
    tcpSendBuffer_(0),
    tcpRecvBuffer_(0),
    keepAlive_(false),
    interruptableChildren_(true),
    listening_(false),
    interruptSockWriter_(-1),
    interruptSockReader_(-1),
    childInterruptSockWriter_(-1) {
}

TServerSocket::~TServerSocket() {
  close();
}

void TServerSocket::setSendTimeout(int sendTimeout) {
  sendTimeout_ = sendTimeout;
}

void TServerSocket::setRecvTimeout(int recvTimeout) {
  recvTimeout_ = recvTimeout;
}

void TServerSocket::setAcceptTimeout(int accTimeout) {
  accTimeout_ = accTimeout;
}

void TServerSocket::setAcceptBacklog(int accBacklog) {
  acceptBacklog_ = accBacklog;
}

void TServerSocket::setRetryLimit(int retryLimit) {
  retryLimit_ = retryLimit;
}

void TServerSocket::setRetryDelay(int retryDelay) {
  retryDelay_ = retryDelay;
}

void TServerSocket::setTcpSendBuffer(int tcpSendBuffer) {
  tcpSendBuffer_ = tcpSendBuffer;
}

void TServerSocket::setTcpRecvBuffer(int tcpRecvBuffer) {
  tcpRecvBuffer_ = tcpRecvBuffer;
}

void TServerSocket::setInterruptableChildren(bool enable) {
  if (listening_) {
    throw std::logic_error("setInterruptableChildren cannot be called after listen()");
  }
  interruptableChildren_ = enable;
}

void TServerSocket::listen() {
  listening_ = true;
  int sv[2];
  // Create the socket pair used to interrupt
  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sv)) {
    RDDPLOG(ERROR) << "TServerSocket::listen() socketpair() interrupt";
    interruptSockWriter_ = -1;
    interruptSockReader_ = -1;
  } else {
    interruptSockWriter_ = sv[1];
    interruptSockReader_ = sv[0];
  }

  // Create the socket pair used to interrupt all clients
  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sv)) {
    RDDPLOG(ERROR) << "TServerSocket::listen() socketpair() childInterrupt";
    childInterruptSockWriter_ = -1;
    pChildInterruptSockReader_.reset();
  } else {
    childInterruptSockWriter_ = sv[1];
    pChildInterruptSockReader_
        = boost::shared_ptr<int>(new int(sv[0]), destroyer_of_fine_sockets);
  }

  // Validate port number
  if (port_ < 0 || port_ > 0xFFFF) {
    throw TTransportException(TTransportException::BAD_ARGS, "Specified port is invalid");
  }

  struct addrinfo hints, *res, *res0;
  int error;
  char port[sizeof("65535")];
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  sprintf(port, "%d", port_);

  // If address is not specified use wildcard address (NULL)
  error = getaddrinfo(address_.empty() ? NULL : &address_[0], port, &hints, &res0);
  if (error) {
    RDDLOG(ERROR) << "getaddrinfo " << error << ": " << gai_strerror(error);
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not resolve host for server socket.");
  }

  // Pick the ipv6 address first since ipv4 addresses can be mapped
  // into ipv6 space.
  for (res = res0; res; res = res->ai_next) {
    if (res->ai_family == AF_INET6 || res->ai_next == NULL)
      break;
  }

  if (!path_.empty()) {
    serverSocket_ = socket(PF_UNIX, SOCK_STREAM, IPPROTO_IP);
  } else {
    serverSocket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  }

  if (serverSocket_ == -1) {
    RDDPLOG(ERROR) << "TServerSocket::listen() socket()";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not create server socket.",
                              errno_copy);
  }

  // Set SO_REUSEADDR to prevent 2MSL delay on accept
  int one = 1;
  if (-1 == setsockopt(serverSocket_,
                       SOL_SOCKET,
                       SO_REUSEADDR,
                       cast_sockopt(&one),
                       sizeof(one))) {
// ignore errors coming out of this setsockopt on Windows.  This is because
// SO_EXCLUSIVEADDRUSE requires admin privileges on WinXP, but we don't
// want to force servers to be an admin.
    RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() SO_REUSEADDR";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not set SO_REUSEADDR",
                              errno_copy);
  }

  // Set TCP buffer sizes
  if (tcpSendBuffer_ > 0) {
    if (-1 == setsockopt(serverSocket_,
                         SOL_SOCKET,
                         SO_SNDBUF,
                         cast_sockopt(&tcpSendBuffer_),
                         sizeof(tcpSendBuffer_))) {
      RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() SO_SNDBUF";
      int errno_copy = errno;
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set SO_SNDBUF",
                                errno_copy);
    }
  }

  if (tcpRecvBuffer_ > 0) {
    if (-1 == setsockopt(serverSocket_,
                         SOL_SOCKET,
                         SO_RCVBUF,
                         cast_sockopt(&tcpRecvBuffer_),
                         sizeof(tcpRecvBuffer_))) {
      RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() SO_RCVBUF";
      int errno_copy = errno;
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set SO_RCVBUF",
                                errno_copy);
    }
  }

// Defer accept
#ifdef TCP_DEFER_ACCEPT
  if (path_.empty()) {
    if (-1 == setsockopt(serverSocket_, IPPROTO_TCP, TCP_DEFER_ACCEPT, &one, sizeof(one))) {
      RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() TCP_DEFER_ACCEPT";
      int errno_copy = errno;
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set TCP_DEFER_ACCEPT",
                                errno_copy);
    }
  }
#endif // #ifdef TCP_DEFER_ACCEPT

#ifdef IPV6_V6ONLY
  if (res->ai_family == AF_INET6 && path_.empty()) {
    int zero = 0;
    if (-1 == setsockopt(serverSocket_,
                         IPPROTO_IPV6,
                         IPV6_V6ONLY,
                         cast_sockopt(&zero),
                         sizeof(zero))) {
      RDDPLOG(ERROR) << "TServerSocket::listen() IPV6_V6ONLY";
    }
  }
#endif // #ifdef IPV6_V6ONLY

  // Turn linger off, don't want to block on calls to close
  struct linger ling = {0, 0};
  if (-1 == setsockopt(serverSocket_, SOL_SOCKET, SO_LINGER, cast_sockopt(&ling), sizeof(ling))) {
    RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() SO_LINGER";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not set SO_LINGER", errno_copy);
  }

  // Unix Sockets do not need that
  if (path_.empty()) {
    // TCP Nodelay, speed over bandwidth
    if (-1
        == setsockopt(serverSocket_, IPPROTO_TCP, TCP_NODELAY, cast_sockopt(&one), sizeof(one))) {
      RDDPLOG(ERROR) << "TServerSocket::listen() setsockopt() TCP_NODELAY";
      int errno_copy = errno;
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set TCP_NODELAY",
                                errno_copy);
    }
  }

  // Set NONBLOCK on the accept socket
  int flags = fcntl(serverSocket_, F_GETFL, 0);
  if (flags == -1) {
    RDDPLOG(ERROR) << "TServerSocket::listen() fcntl() F_GETFL";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "fcntl() F_GETFL failed",
                              errno_copy);
  }

  if (-1 == fcntl(serverSocket_, F_SETFL, flags | O_NONBLOCK)) {
    RDDPLOG(ERROR) << "TServerSocket::listen() fcntl() O_NONBLOCK";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "fcntl() F_SETFL O_NONBLOCK failed",
                              errno_copy);
  }

  // prepare the port information
  // we may want to try to bind more than once, since SO_REUSEADDR doesn't
  // always seem to work. The client can configure the retry variables.
  int retries = 0;

  if (!path_.empty()) {
    // Unix Domain Socket
    size_t len = path_.size() + 1;
    if (len > sizeof(((sockaddr_un*)NULL)->sun_path)) {
      RDDPLOG(ERROR) << "TSocket::listen() Unix Domain socket path too long";
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Unix Domain socket path too long",
                                errno);
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    memcpy(address.sun_path, path_.c_str(), len);
    socklen_t structlen = static_cast<socklen_t>(sizeof(address));

    do {
      if (0 == ::bind(serverSocket_, (struct sockaddr*)&address, structlen)) {
        break;
      }
      // use short circuit evaluation here to only sleep if we need to
    } while ((retries++ < retryLimit_) && (sleep(retryDelay_) == 0));
  } else {
    do {
      if (0 == ::bind(serverSocket_, res->ai_addr, static_cast<int>(res->ai_addrlen))) {
        break;
      }
      // use short circuit evaluation here to only sleep if we need to
    } while ((retries++ < retryLimit_) && (sleep(retryDelay_) == 0));

    // free addrinfo
    freeaddrinfo(res0);

    // retrieve bind info
    if (port_ == 0 && retries <= retryLimit_) {
      struct sockaddr sa;
      socklen_t len = sizeof(sa);
      std::memset(&sa, 0, len);
      if (::getsockname(serverSocket_, &sa, &len) < 0) {
        RDDPLOG(ERROR) << "TServerSocket::getPort() getsockname()";
      } else {
        if (sa.sa_family == AF_INET6) {
          const struct sockaddr_in6* sin = reinterpret_cast<const struct sockaddr_in6*>(&sa);
          port_ = ntohs(sin->sin6_port);
        } else {
          const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(&sa);
          port_ = ntohs(sin->sin_port);
        }
      }
    }
  }

  // throw an error if we failed to bind properly
  if (retries > retryLimit_) {
    if (!path_.empty()) {
      RDDLOG(ERROR) << "TServerSocket::listen() PATH " << path_;
    } else {
      RDDLOG(ERROR) << "TServerSocket::listen() BIND " << port_;
    }
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not bind",
                              errno);
  }

  if (listenCallback_)
    listenCallback_(serverSocket_);

  // Call listen
  if (-1 == ::listen(serverSocket_, acceptBacklog_)) {
    RDDPLOG(ERROR) << "TServerSocket::listen() listen()";
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not listen", errno_copy);
  }

  // The socket is now listening!
}

int TServerSocket::getPort() {
  return port_;
}

shared_ptr<TTransport> TServerSocket::acceptImpl() {
  if (serverSocket_ == -1) {
    throw TTransportException(TTransportException::NOT_OPEN, "TServerSocket not listening");
  }

  struct pollfd fds[2];

  int maxEintrs = 5;
  int numEintrs = 0;

  while (true) {
    std::memset(fds, 0, sizeof(fds));
    fds[0].fd = serverSocket_;
    fds[0].events = POLLIN;
    if (interruptSockReader_ != -1) {
      fds[1].fd = interruptSockReader_;
      fds[1].events = POLLIN;
    }
    /*
      TODO: if EINTR is received, we'll restart the timeout.
      To be accurate, we need to fix this in the future.
     */
    int ret = poll(fds, 2, accTimeout_);

    if (ret < 0) {
      // error cases
      if (errno == EINTR && (numEintrs++ < maxEintrs)) {
        // EINTR needs to be handled manually and we can tolerate
        // a certain number
        continue;
      }
      RDDPLOG(ERROR) << "TServerSocket::acceptImpl() poll()";
      throw TTransportException(TTransportException::UNKNOWN, "Unknown", errno);
    } else if (ret > 0) {
      // Check for an interrupt signal
      if (interruptSockReader_ != -1 && (fds[1].revents & POLLIN)) {
        int8_t buf;
        if (-1 == recv(interruptSockReader_, cast_sockopt(&buf), sizeof(int8_t), 0)) {
          RDDPLOG(ERROR) << "TServerSocket::acceptImpl() recv() interrupt";
        }
        throw TTransportException(TTransportException::INTERRUPTED);
      }

      // Check for the actual server socket being ready
      if (fds[0].revents & POLLIN) {
        break;
      }
    } else {
      RDDPLOG(ERROR) << "TServerSocket::acceptImpl() poll 0";
      throw TTransportException(TTransportException::UNKNOWN);
    }
  }

  struct sockaddr_storage clientAddress;
  int size = sizeof(clientAddress);
  int clientSocket
      = ::accept(serverSocket_, (struct sockaddr*)&clientAddress, (socklen_t*)&size);

  if (clientSocket == -1) {
    RDDPLOG(ERROR) << "TServerSocket::acceptImpl() ::accept()";
    throw TTransportException(TTransportException::UNKNOWN, "accept()", errno);
  }

  // Make sure client socket is blocking
  int flags = fcntl(clientSocket, F_GETFL, 0);
  if (flags == -1) {
    RDDPLOG(ERROR) << "TServerSocket::acceptImpl() fcntl() F_GETFL";
    int errno_copy = errno;
    ::close(clientSocket);
    throw TTransportException(TTransportException::UNKNOWN,
                              "fcntl(F_GETFL)",
                              errno_copy);
  }

  if (-1 == fcntl(clientSocket, F_SETFL, flags & ~O_NONBLOCK)) {
    RDDPLOG(ERROR) << "TServerSocket::acceptImpl() fcntl() F_SETFL ~O_NONBLOCK";
    int errno_copy = errno;
    ::close(clientSocket);
    throw TTransportException(TTransportException::UNKNOWN,
                              "fcntl(F_SETFL)",
                              errno_copy);
  }

  shared_ptr<TSocket> client = createSocket(clientSocket);
  if (sendTimeout_ > 0) {
    client->setSendTimeout(sendTimeout_);
  }
  if (recvTimeout_ > 0) {
    client->setRecvTimeout(recvTimeout_);
  }
  if (keepAlive_) {
    client->setKeepAlive(keepAlive_);
  }
  client->setCachedAddress((sockaddr*)&clientAddress, size);

  if (acceptCallback_)
    acceptCallback_(clientSocket);

  return client;
}

shared_ptr<TSocket> TServerSocket::createSocket(int clientSocket) {
  if (interruptableChildren_) {
    return shared_ptr<TSocket>(new TSocket(clientSocket, pChildInterruptSockReader_));
  } else {
    return shared_ptr<TSocket>(new TSocket(clientSocket));
  }
}

void TServerSocket::notify(int notifySocket) {
  if (notifySocket != -1) {
    int8_t byte = 0;
    if (-1 == send(notifySocket, cast_sockopt(&byte), sizeof(int8_t), 0)) {
      RDDPLOG(ERROR) << "TServerSocket::notify() send()";
    }
  }
}

void TServerSocket::interrupt() {
  notify(interruptSockWriter_);
}

void TServerSocket::interruptChildren() {
  notify(childInterruptSockWriter_);
}

void TServerSocket::close() {
  if (serverSocket_ != -1) {
    shutdown(serverSocket_, SHUT_RDWR);
    ::close(serverSocket_);
  }
  if (interruptSockWriter_ != -1) {
    ::close(interruptSockWriter_);
  }
  if (interruptSockReader_ != -1) {
    ::close(interruptSockReader_);
  }
  if (childInterruptSockWriter_ != -1) {
    ::close(childInterruptSockWriter_);
  }
  serverSocket_ = -1;
  interruptSockWriter_ = -1;
  interruptSockReader_ = -1;
  childInterruptSockWriter_ = -1;
  pChildInterruptSockReader_.reset();
  listening_ = false;
}
}
}
} // apache::thrift::transport
