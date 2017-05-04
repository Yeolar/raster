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
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportException.h>
#include "rddoc/util/Logging.h"

template <class T>
inline const void* const_cast_sockopt(const T* v) {
  return reinterpret_cast<const void*>(v);
}

template <class T>
inline void* cast_sockopt(T* v) {
  return reinterpret_cast<void*>(v);
}

namespace apache {
namespace thrift {
namespace transport {

using namespace std;

/**
 * TSocket implementation.
 *
 */

TSocket::TSocket(const string& host, int port)
  : host_(host),
    port_(port),
    path_(""),
    socket_(-1),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {
}

TSocket::TSocket(const string& path)
  : host_(""),
    port_(0),
    path_(path),
    socket_(-1),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {
  cachedPeerAddr_.ipv4.sin_family = AF_UNSPEC;
}

TSocket::TSocket()
  : host_(""),
    port_(0),
    path_(""),
    socket_(-1),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {
  cachedPeerAddr_.ipv4.sin_family = AF_UNSPEC;
}

TSocket::TSocket(int socket)
  : host_(""),
    port_(0),
    path_(""),
    socket_(socket),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {
  cachedPeerAddr_.ipv4.sin_family = AF_UNSPEC;
#ifdef SO_NOSIGPIPE
  {
    int one = 1;
    setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
  }
#endif
}

TSocket::TSocket(int socket, boost::shared_ptr<int> interruptListener)
  : host_(""),
    port_(0),
    path_(""),
    socket_(socket),
    interruptListener_(interruptListener),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {
  cachedPeerAddr_.ipv4.sin_family = AF_UNSPEC;
#ifdef SO_NOSIGPIPE
  {
    int one = 1;
    setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
  }
#endif
}

TSocket::~TSocket() {
  close();
}

bool TSocket::isOpen() {
  return (socket_ != -1);
}

bool TSocket::peek() {
  if (!isOpen()) {
    return false;
  }
  if (interruptListener_) {
    for (int retries = 0;;) {
      struct pollfd fds[2];
      std::memset(fds, 0, sizeof(fds));
      fds[0].fd = socket_;
      fds[0].events = POLLIN;
      fds[1].fd = *(interruptListener_.get());
      fds[1].events = POLLIN;
      int ret = poll(fds, 2, (recvTimeout_ == 0) ? -1 : recvTimeout_);
      if (ret < 0) {
        // error cases
        if (errno == EINTR && (retries++ < maxRecvRetries_)) {
          continue;
        }
        RDDPLOG(ERROR) << "TSocket::peek() poll()";
        throw TTransportException(TTransportException::UNKNOWN, "Unknown", errno);
      } else if (ret > 0) {
        // Check the interruptListener
        if (fds[1].revents & POLLIN) {
          return false;
        }
        // There must be data or a disconnection, fall through to the PEEK
        break;
      } else {
        // timeout
        return false;
      }
    }
  }

  // Check to see if data is available or if the remote side closed
  uint8_t buf;
  int r = static_cast<int>(recv(socket_, cast_sockopt(&buf), 1, MSG_PEEK));
  if (r == -1) {
#if defined __FreeBSD__ || defined __MACH__
    /* shigin:
     * freebsd returns -1 and ECONNRESET if socket was closed by
     * the other side
     */
    if (errno == ECONNRESET) {
      close();
      return false;
    }
#endif
    RDDPLOG(ERROR) << "TSocket::peek() recv() " << getSocketInfo();
    throw TTransportException(TTransportException::UNKNOWN, "recv()", errno);
  }
  return (r > 0);
}

void TSocket::openConnection(struct addrinfo* res) {

  if (isOpen()) {
    return;
  }

  if (!path_.empty()) {
    socket_ = socket(PF_UNIX, SOCK_STREAM, IPPROTO_IP);
  } else {
    socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  }

  if (socket_ == -1) {
    RDDPLOG(ERROR) << "TSocket::open() socket() " << getSocketInfo();
    throw TTransportException(TTransportException::NOT_OPEN, "socket()", errno);
  }

  // Send timeout
  if (sendTimeout_ > 0) {
    setSendTimeout(sendTimeout_);
  }

  // Recv timeout
  if (recvTimeout_ > 0) {
    setRecvTimeout(recvTimeout_);
  }

  if (keepAlive_) {
    setKeepAlive(keepAlive_);
  }

  // Linger
  setLinger(lingerOn_, lingerVal_);

  // No delay
  setNoDelay(noDelay_);

#ifdef SO_NOSIGPIPE
  {
    int one = 1;
    setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
  }
#endif

// Uses a low min RTO if asked to.
#ifdef TCP_LOW_MIN_RTO
  if (getUseLowMinRto()) {
    int one = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_LOW_MIN_RTO, &one, sizeof(one));
  }
#endif

  // Set the socket to be non blocking for connect if a timeout exists
  int flags = fcntl(socket_, F_GETFL, 0);
  if (connTimeout_ > 0) {
    if (-1 == fcntl(socket_, F_SETFL, flags | O_NONBLOCK)) {
      RDDPLOG(ERROR) << "TSocket::open() fcntl() " << getSocketInfo();
      throw TTransportException(TTransportException::NOT_OPEN, "fcntl() failed", errno);
    }
  } else {
    if (-1 == fcntl(socket_, F_SETFL, flags & ~O_NONBLOCK)) {
      RDDPLOG(ERROR) << "TSocket::open() fcntl " << getSocketInfo();
      throw TTransportException(TTransportException::NOT_OPEN, "fcntl() failed", errno);
    }
  }

  // Connect the socket
  int ret;
  if (!path_.empty()) {
    size_t len = path_.size() + 1;
    if (len > sizeof(((sockaddr_un*)NULL)->sun_path)) {
      RDDPLOG(ERROR) << "TSocket::open() Unix Domain socket path too long";
      throw TTransportException(TTransportException::NOT_OPEN, " Unix Domain socket path too long");
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    memcpy(address.sun_path, path_.c_str(), len);
    socklen_t structlen = static_cast<socklen_t>(sizeof(address));
    ret = connect(socket_, (struct sockaddr*)&address, structlen);
  } else {
    ret = connect(socket_, res->ai_addr, static_cast<int>(res->ai_addrlen));
  }

  // success case
  if (ret == 0) {
    goto done;
  }

  if ((errno != EINPROGRESS) && (errno != EWOULDBLOCK)) {
    RDDPLOG(ERROR) << "TSocket::open() connect() " << getSocketInfo();
    throw TTransportException(TTransportException::NOT_OPEN, "connect() failed", errno);
  }

  struct pollfd fds[1];
  std::memset(fds, 0, sizeof(fds));
  fds[0].fd = socket_;
  fds[0].events = POLLOUT;
  ret = poll(fds, 1, connTimeout_);

  if (ret > 0) {
    // Ensure the socket is connected and that there are no errors set
    int val;
    socklen_t lon;
    lon = sizeof(int);
    int ret2 = getsockopt(socket_, SOL_SOCKET, SO_ERROR, cast_sockopt(&val), &lon);
    if (ret2 == -1) {
      RDDPLOG(ERROR) << "TSocket::open() getsockopt() " << getSocketInfo();
      throw TTransportException(TTransportException::NOT_OPEN, "getsockopt()", errno);
    }
    // no errors on socket, go to town
    if (val == 0) {
      goto done;
    }
    RDDLOG(ERROR) << "TSocket::open() error on socket (after poll) "
      << getSocketInfo() << rdd::errnoStr(val);
    throw TTransportException(TTransportException::NOT_OPEN, "socket open() error", val);
  } else if (ret == 0) {
    // socket timed out
    RDDLOG(ERROR) << "TSocket::open() timed out " << getSocketInfo();
    throw TTransportException(TTransportException::NOT_OPEN, "open() timed out");
  } else {
    // error on poll()
    RDDPLOG(ERROR) << "TSocket::open() poll() " + getSocketInfo();
    throw TTransportException(TTransportException::NOT_OPEN, "poll() failed", errno);
  }

done:
  // Set socket back to normal mode (blocking)
  fcntl(socket_, F_SETFL, flags);

  if (path_.empty()) {
    setCachedAddress(res->ai_addr, static_cast<socklen_t>(res->ai_addrlen));
  }
}

void TSocket::open() {
  if (isOpen()) {
    return;
  }
  if (!path_.empty()) {
    unix_open();
  } else {
    local_open();
  }
}

void TSocket::unix_open() {
  if (!path_.empty()) {
    // Unix Domain SOcket does not need addrinfo struct, so we pass NULL
    openConnection(NULL);
  }
}

void TSocket::local_open() {
  if (isOpen()) {
    return;
  }

  // Validate port number
  if (port_ < 0 || port_ > 0xFFFF) {
    throw TTransportException(TTransportException::BAD_ARGS, "Specified port is invalid");
  }

  struct addrinfo hints, *res, *res0;
  res = NULL;
  res0 = NULL;
  int error;
  char port[sizeof("65535")];
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  sprintf(port, "%d", port_);

  error = getaddrinfo(host_.c_str(), port, &hints, &res0);

  if (error) {
    RDDLOG(ERROR) << "TSocket::open() getaddrinfo() "
      << getSocketInfo() << gai_strerror(error);
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not resolve host for client socket.");
  }

  // Cycle through all the returned addresses until one
  // connects or push the exception up.
  for (res = res0; res; res = res->ai_next) {
    try {
      openConnection(res);
      break;
    } catch (TTransportException&) {
      if (res->ai_next) {
        close();
      } else {
        close();
        freeaddrinfo(res0); // cleanup on failure
        throw;
      }
    }
  }

  // Free address structure memory
  freeaddrinfo(res0);
}

void TSocket::close() {
  if (socket_ != -1) {
    shutdown(socket_, SHUT_RDWR);
    ::close(socket_);
  }
  socket_ = -1;
}

void TSocket::setSocketFD(int socket) {
  if (socket_ != -1) {
    close();
  }
  socket_ = socket;
}

uint32_t TSocket::read(uint8_t* buf, uint32_t len) {
  if (socket_ == -1) {
    throw TTransportException(TTransportException::NOT_OPEN, "Called read on non-open socket");
  }

  int32_t retries = 0;

  // EAGAIN can be signalled both when a timeout has occurred and when
  // the system is out of resources (an awesome undocumented feature).
  // The following is an approximation of the time interval under which
  // EAGAIN is taken to indicate an out of resources error.
  uint32_t eagainThresholdMicros = 0;
  if (recvTimeout_) {
    // if a readTimeout is specified along with a max number of recv retries, then
    // the threshold will ensure that the read timeout is not exceeded even in the
    // case of resource errors
    eagainThresholdMicros = (recvTimeout_ * 1000) / ((maxRecvRetries_ > 0) ? maxRecvRetries_ : 2);
  }

try_again:
  // Read from the socket
  struct timeval begin;
  if (recvTimeout_ > 0) {
    gettimeofday(&begin, NULL);
  } else {
    // if there is no read timeout we don't need the TOD to determine whether
    // an EAGAIN is due to a timeout or an out-of-resource condition.
    begin.tv_sec = begin.tv_usec = 0;
  }

  int got = 0;

  if (interruptListener_) {
    struct pollfd fds[2];
    std::memset(fds, 0, sizeof(fds));
    fds[0].fd = socket_;
    fds[0].events = POLLIN;
    fds[1].fd = *(interruptListener_.get());
    fds[1].events = POLLIN;

    int ret = poll(fds, 2, (recvTimeout_ == 0) ? -1 : recvTimeout_);
    if (ret < 0) {
      // error cases
      if (errno == EINTR && (retries++ < maxRecvRetries_)) {
        goto try_again;
      }
      RDDPLOG(ERROR) << "TSocket::read() poll()";
      throw TTransportException(TTransportException::UNKNOWN, "Unknown", errno);
    } else if (ret > 0) {
      // Check the interruptListener
      if (fds[1].revents & POLLIN) {
        throw TTransportException(TTransportException::INTERRUPTED, "Interrupted");
      }
    } else /* ret == 0 */ {
      throw TTransportException(TTransportException::TIMED_OUT, "EAGAIN (timed out)");
    }

    // falling through means there is something to recv and it cannot block
  }

  got = static_cast<int>(recv(socket_, cast_sockopt(buf), len, 0));

  // Check for error on read
  if (got < 0) {
    if (errno == EAGAIN) {
      // if no timeout we can assume that resource exhaustion has occurred.
      if (recvTimeout_ == 0) {
        throw TTransportException(TTransportException::TIMED_OUT,
                                  "EAGAIN (unavailable resources)");
      }
      // check if this is the lack of resources or timeout case
      struct timeval end;
      gettimeofday(&end, NULL);
      uint32_t readElapsedMicros = static_cast<uint32_t>(((end.tv_sec - begin.tv_sec) * 1000 * 1000)
                                                         + (end.tv_usec - begin.tv_usec));

      if (!eagainThresholdMicros || (readElapsedMicros < eagainThresholdMicros)) {
        if (retries++ < maxRecvRetries_) {
          usleep(50);
          goto try_again;
        } else {
          throw TTransportException(TTransportException::TIMED_OUT,
                                    "EAGAIN (unavailable resources)");
        }
      } else {
        // infer that timeout has been hit
        throw TTransportException(TTransportException::TIMED_OUT, "EAGAIN (timed out)");
      }
    }

    // If interrupted, try again
    if (errno == EINTR && retries++ < maxRecvRetries_) {
      goto try_again;
    }

    if (errno == ECONNRESET) {
      return 0;
    }

    // This ish isn't open
    if (errno == ENOTCONN) {
      throw TTransportException(TTransportException::NOT_OPEN, "ENOTCONN");
    }

    // Timed out!
    if (errno == ETIMEDOUT) {
      throw TTransportException(TTransportException::TIMED_OUT, "ETIMEDOUT");
    }

    // Now it's not a try again case, but a real probblez
    RDDPLOG(ERROR) << "TSocket::read() recv() " << getSocketInfo();

    // Some other error, whatevz
    throw TTransportException(TTransportException::UNKNOWN, "Unknown", errno);
  }

  return got;
}

void TSocket::write(const uint8_t* buf, uint32_t len) {
  uint32_t sent = 0;

  while (sent < len) {
    uint32_t b = write_partial(buf + sent, len - sent);
    if (b == 0) {
      // This should only happen if the timeout set with SO_SNDTIMEO expired.
      // Raise an exception.
      throw TTransportException(TTransportException::TIMED_OUT, "send timeout expired");
    }
    sent += b;
  }
}

uint32_t TSocket::write_partial(const uint8_t* buf, uint32_t len) {
  if (socket_ == -1) {
    throw TTransportException(TTransportException::NOT_OPEN, "Called write on non-open socket");
  }

  uint32_t sent = 0;

  int flags = 0;
#ifdef MSG_NOSIGNAL
  // Note the use of MSG_NOSIGNAL to suppress SIGPIPE errors, instead we
  // check for the EPIPE return condition and close the socket in that case
  flags |= MSG_NOSIGNAL;
#endif // ifdef MSG_NOSIGNAL

  int b = static_cast<int>(send(socket_, const_cast_sockopt(buf + sent), len - sent, flags));

  if (b < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      return 0;
    }
    // Fail on a send error
    RDDPLOG(ERROR) << "TSocket::write_partial() send() " << getSocketInfo();

    if (errno == EPIPE || errno == ECONNRESET || errno == ENOTCONN) {
      close();
      throw TTransportException(TTransportException::NOT_OPEN, "write() send()", errno);
    }

    throw TTransportException(TTransportException::UNKNOWN, "write() send()", errno);
  }

  // Fail on blocked send
  if (b == 0) {
    throw TTransportException(TTransportException::NOT_OPEN, "Socket send returned 0.");
  }
  return b;
}

std::string TSocket::getHost() {
  return host_;
}

int TSocket::getPort() {
  return port_;
}

void TSocket::setHost(string host) {
  host_ = host;
}

void TSocket::setPort(int port) {
  port_ = port;
}

void TSocket::setLinger(bool on, int linger) {
  lingerOn_ = on;
  lingerVal_ = linger;
  if (socket_ == -1) {
    return;
  }

  struct linger l = {(lingerOn_ ? 1 : 0), lingerVal_};
  int ret = setsockopt(socket_, SOL_SOCKET, SO_LINGER, cast_sockopt(&l), sizeof(l));
  if (ret == -1) {
    RDDPLOG(ERROR) << "TSocket::setLinger() setsockopt() " << getSocketInfo();
  }
}

void TSocket::setNoDelay(bool noDelay) {
  noDelay_ = noDelay;
  if (socket_ == -1 || !path_.empty()) {
    return;
  }

  // Set socket to NODELAY
  int v = noDelay_ ? 1 : 0;
  int ret = setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, cast_sockopt(&v), sizeof(v));
  if (ret == -1) {
    RDDPLOG(ERROR) << "TSocket::setNoDelay() setsockopt() " << getSocketInfo();
  }
}

void TSocket::setConnTimeout(int ms) {
  connTimeout_ = ms;
}

void setGenericTimeout(int s, int timeout_ms, int optname) {
  if (timeout_ms < 0) {
    RDDLOG(ERROR) << "TSocket::setGenericTimeout with negative input: " << timeout_ms;
    return;
  }

  if (s == -1) {
    return;
  }

  struct timeval platform_time = {(int)(timeout_ms / 1000), (int)((timeout_ms % 1000) * 1000)};
  int ret = setsockopt(s, SOL_SOCKET, optname, cast_sockopt(&platform_time), sizeof(platform_time));
  if (ret == -1) {
    RDDPLOG(ERROR) << "TSocket::setGenericTimeout() setsockopt()";
  }
}

void TSocket::setRecvTimeout(int ms) {
  setGenericTimeout(socket_, ms, SO_RCVTIMEO);
  recvTimeout_ = ms;
}

void TSocket::setSendTimeout(int ms) {
  setGenericTimeout(socket_, ms, SO_SNDTIMEO);
  sendTimeout_ = ms;
}

void TSocket::setKeepAlive(bool keepAlive) {
  keepAlive_ = keepAlive;

  if (socket_ == -1) {
    return;
  }

  int value = keepAlive_;
  int ret
      = setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, const_cast_sockopt(&value), sizeof(value));

  if (ret == -1) {
    RDDPLOG(ERROR) << "TSocket::setKeepAlive() setsockopt() " << getSocketInfo();
  }
}

void TSocket::setMaxRecvRetries(int maxRecvRetries) {
  maxRecvRetries_ = maxRecvRetries;
}

string TSocket::getSocketInfo() {
  std::ostringstream oss;
  if (host_.empty() || port_ == 0) {
    oss << "<Host: " << getPeerAddress();
    oss << " Port: " << getPeerPort() << ">";
  } else {
    oss << "<Host: " << host_ << " Port: " << port_ << ">";
  }
  return oss.str();
}

std::string TSocket::getPeerHost() {
  if (peerHost_.empty() && path_.empty()) {
    struct sockaddr_storage addr;
    struct sockaddr* addrPtr;
    socklen_t addrLen;

    if (socket_ == -1) {
      return host_;
    }

    addrPtr = getCachedAddress(&addrLen);

    if (addrPtr == NULL) {
      addrLen = sizeof(addr);
      if (getpeername(socket_, (sockaddr*)&addr, &addrLen) != 0) {
        return peerHost_;
      }
      addrPtr = (sockaddr*)&addr;

      setCachedAddress(addrPtr, addrLen);
    }

    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];

    getnameinfo((sockaddr*)addrPtr,
                addrLen,
                clienthost,
                sizeof(clienthost),
                clientservice,
                sizeof(clientservice),
                0);

    peerHost_ = clienthost;
  }
  return peerHost_;
}

std::string TSocket::getPeerAddress() {
  if (peerAddress_.empty() && path_.empty()) {
    struct sockaddr_storage addr;
    struct sockaddr* addrPtr;
    socklen_t addrLen;

    if (socket_ == -1) {
      return peerAddress_;
    }

    addrPtr = getCachedAddress(&addrLen);

    if (addrPtr == NULL) {
      addrLen = sizeof(addr);
      if (getpeername(socket_, (sockaddr*)&addr, &addrLen) != 0) {
        return peerAddress_;
      }
      addrPtr = (sockaddr*)&addr;

      setCachedAddress(addrPtr, addrLen);
    }

    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];

    getnameinfo(addrPtr,
                addrLen,
                clienthost,
                sizeof(clienthost),
                clientservice,
                sizeof(clientservice),
                NI_NUMERICHOST | NI_NUMERICSERV);

    peerAddress_ = clienthost;
    peerPort_ = std::atoi(clientservice);
  }
  return peerAddress_;
}

int TSocket::getPeerPort() {
  getPeerAddress();
  return peerPort_;
}

void TSocket::setCachedAddress(const sockaddr* addr, socklen_t len) {
  if (!path_.empty()) {
    return;
  }

  switch (addr->sa_family) {
  case AF_INET:
    if (len == sizeof(sockaddr_in)) {
      memcpy((void*)&cachedPeerAddr_.ipv4, (void*)addr, len);
    }
    break;

  case AF_INET6:
    if (len == sizeof(sockaddr_in6)) {
      memcpy((void*)&cachedPeerAddr_.ipv6, (void*)addr, len);
    }
    break;
  }
  peerAddress_.clear();
  peerHost_.clear();
}

sockaddr* TSocket::getCachedAddress(socklen_t* len) const {
  switch (cachedPeerAddr_.ipv4.sin_family) {
  case AF_INET:
    *len = sizeof(sockaddr_in);
    return (sockaddr*)&cachedPeerAddr_.ipv4;

  case AF_INET6:
    *len = sizeof(sockaddr_in6);
    return (sockaddr*)&cachedPeerAddr_.ipv6;

  default:
    return NULL;
  }
}

bool TSocket::useLowMinRto_ = false;
void TSocket::setUseLowMinRto(bool useLowMinRto) {
  useLowMinRto_ = useLowMinRto;
}
bool TSocket::getUseLowMinRto() {
  return useLowMinRto_;
}

const std::string TSocket::getOrigin() {
  std::ostringstream oss;
  oss << getPeerHost() << ":" << getPeerPort();
  return oss.str();
}
}
}
} // apache::thrift::transport
