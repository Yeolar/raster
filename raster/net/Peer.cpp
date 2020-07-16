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

#include "raster/net/Peer.h"

#include <cinttypes>
#include <net/if.h>

#include <accelerator/Conv.h>
#include <accelerator/Exception.h>
#include <accelerator/Hash.h>
#include <accelerator/Macro.h>

namespace {

struct ScopedAddrInfo {
  explicit ScopedAddrInfo(struct addrinfo* addrinfo)
      : info(addrinfo) {}

  ~ScopedAddrInfo() {
    freeaddrinfo(info);
  }

  struct addrinfo* info;
};

struct HostAndPort {
  HostAndPort(const char* str, bool hostRequired)
    : host(nullptr), port(nullptr), allocated(nullptr) {
    // Look for the last colon
    const char* colon = strrchr(str, ':');
    if (colon == nullptr) {
      // No colon, just a port number.
      if (hostRequired) {
        throw std::invalid_argument(
            "expected a host and port string of the form \"<host>:<port>\"");
      }
      port = str;
      return;
    }

    // We have to make a copy of the string so we can modify it
    // and change the colon to a NUL terminator.
    allocated = strdup(str);
    if (!allocated) {
      throw std::bad_alloc();
    }

    char* allocatedColon = allocated + (colon - str);
    *allocatedColon = '\0';
    host = allocated;
    port = allocatedColon + 1;
    // bracketed IPv6 address, remove the brackets
    // allocatedColon[-1] is fine, as allocatedColon >= host and
    // *allocatedColon != *host therefore allocatedColon > host
    if (*host == '[' && allocatedColon[-1] == ']') {
      allocatedColon[-1] = '\0';
      ++host;
    }
  }

  ~HostAndPort() {
    free(allocated);
  }

  const char* host;
  const char* port;
  char* allocated;
};

// create an in_addr from an uint8_t*
inline in_addr mkAddress4(const uint8_t* src) {
  union {
    in_addr addr;
    uint8_t bytes[4];
  } addr;
  ::memset(&addr, 0, 4);
  ::memcpy(addr.bytes, src, 4);
  return addr.addr;
}

// create an in6_addr from an uint8_t*
inline in6_addr mkAddress6(const uint8_t* src) {
  in6_addr addr;
  ::memset(&addr, 0, 16);
  ::memcpy(addr.s6_addr, src, 16);
  return addr;
}

template <
    class IntegralType,
    IntegralType DigitCount,
    IntegralType Base = IntegralType(10),
    bool PrintAllDigits = false,
    class = typename std::enable_if<
        std::is_integral<IntegralType>::value &&
            std::is_unsigned<IntegralType>::value,
        bool>::type>
inline void writeIntegerString(IntegralType val, char** buffer) {
  char* buf = *buffer;

  if (!PrintAllDigits && val == 0) {
    *(buf++) = '0';
    *buffer = buf;
    return;
  }

  IntegralType powerToPrint = 1;
  for (IntegralType i = 1; i < DigitCount; ++i) {
    powerToPrint *= Base;
  }

  bool found = PrintAllDigits;
  while (powerToPrint) {
    if (found || powerToPrint <= val) {
      IntegralType value = IntegralType(val / powerToPrint);
      if (Base == 10 || value < 10) {
        value += '0';
      } else {
        value += ('a' - 10);
      }
      *(buf++) = char(value);
      val %= powerToPrint;
      found = true;
    }

    powerToPrint /= Base;
  }

  *buffer = buf;
}

inline size_t fastIpV4ToBufferUnsafe(const in_addr& inAddr, char* str) {
  const uint8_t* octets = reinterpret_cast<const uint8_t*>(&inAddr.s_addr);
  char* buf = str;

  writeIntegerString<uint8_t, 3>(octets[0], &buf);
  *(buf++) = '.';
  writeIntegerString<uint8_t, 3>(octets[1], &buf);
  *(buf++) = '.';
  writeIntegerString<uint8_t, 3>(octets[2], &buf);
  *(buf++) = '.';
  writeIntegerString<uint8_t, 3>(octets[3], &buf);

  return buf - str;
}

inline std::string fastIpv4ToString(const in_addr& inAddr) {
  char str[sizeof("255.255.255.255")];
  return std::string(str, fastIpV4ToBufferUnsafe(inAddr, str));
}

inline void fastIpv4AppendToString(const in_addr& inAddr, std::string& out) {
  char str[sizeof("255.255.255.255")];
  out.append(str, fastIpV4ToBufferUnsafe(inAddr, str));
}

inline size_t fastIpv6ToBufferUnsafe(const in6_addr& in6Addr, char* str) {
  const uint16_t* bytes = reinterpret_cast<const uint16_t*>(&in6Addr.s6_addr16);
  char* buf = str;

  for (int i = 0; i < 8; ++i) {
    writeIntegerString<
        uint16_t,
        4, // at most 4 hex digits per ushort
        16, // base 16 (hex)
        true>(htons(bytes[i]), &buf);

    if (i != 7) {
      *(buf++) = ':';
    }
  }

  return buf - str;
}

inline std::string fastIpv6ToString(const in6_addr& in6Addr) {
  char str[sizeof("2001:0db8:0000:0000:0000:ff00:0042:8329")];
  return std::string(str, fastIpv6ToBufferUnsafe(in6Addr, str));
}

inline void fastIpv6AppendToString(const in6_addr& in6Addr, std::string& out) {
  char str[sizeof("2001:0db8:0000:0000:0000:ff00:0042:8329")];
  out.append(str, fastIpv6ToBufferUnsafe(in6Addr, str));
}

}

namespace raster {

std::string IPAddressV4::str() const {
  return fastIpv4ToString(addr_.inAddr_);
}

void IPAddressV4::toFullyQualifiedAppend(std::string& out) const {
  fastIpv4AppendToString(addr_.inAddr_, out);
}

size_t IPAddressV4::hash() const {
  return acc::hash::hash_combine(AF_INET, acc::hash::fnv32_buf(&addr_, 4));
}

bool IPAddressV6::isIPv4Mapped() const {
  // v4 mapped addresses have their first 10 bytes set to 0, the next 2 bytes
  // set to 255 (0xff);
  const unsigned char* by = bytes();

  // check if first 10 bytes are 0
  for (int i = 0; i < 10; i++) {
    if (by[i] != 0x00) {
      return false;
    }
  }
  // check if bytes 11 and 12 are 255
  if (by[10] == 0xff && by[11] == 0xff) {
    return true;
  }
  return false;
}

IPAddressV4 IPAddressV6::createIPv4() const {
  if (!isIPv4Mapped()) {
    throw std::runtime_error("addr is not v4-to-v6-mapped");
  }
  const unsigned char* by = bytes();
  return IPAddressV4(mkAddress4(&by[12]));
}

std::string IPAddressV6::str() const {
  char buffer[INET6_ADDRSTRLEN + IFNAMSIZ + 1];

  if (!inet_ntop(AF_INET6, toAddr().s6_addr, buffer, INET6_ADDRSTRLEN)) {
    throw std::runtime_error(
        acc::to<std::string>("Invalid address with error ", strerror(errno)));
  }

  auto scopeId = scope_;
  if (scopeId != 0) {
    auto len = strlen(buffer);
    buffer[len] = '%';

    auto errsv = errno;
    if (!if_indextoname(scopeId, buffer + len + 1)) {
      // if we can't map the if because eg. it no longer exists,
      // append the if index instead
      snprintf(buffer + len + 1, IFNAMSIZ, "%u", scopeId);
    }
    errno = errsv;
  }

  return std::string(buffer);
}

std::string IPAddressV6::toFullyQualified() const {
  return fastIpv6ToString(addr_.in6Addr_);
}

void IPAddressV6::toFullyQualifiedAppend(std::string& out) const {
  fastIpv6AppendToString(addr_.in6Addr_, out);
}

size_t IPAddressV6::hash() const {
  if (isIPv4Mapped()) {
    /* An IPAddress containing this object would be equal (i.e. operator==)
       to an IPAddress containing the corresponding IPv4.
       So we must make sure that the hash values are the same as well */
    return createIPv4().hash();
  }

  uint64_t hash1 = 0, hash2 = 0;
  acc::hash::SpookyHashV2::Hash128(&addr_, 16, &hash1, &hash2);
  return acc::hash::hash_combine(AF_INET6, hash1, hash2);
}

int Peer::getPortFrom(const struct sockaddr* address) {
  switch (address->sa_family) {
    case AF_INET:
      return ntohs(((sockaddr_in*)address)->sin_port);
    case AF_INET6:
      return ntohs(((sockaddr_in6*)address)->sin6_port);
    default:
      return -1;
  }
}

const char* Peer::getFamilyNameFrom(
    const struct sockaddr* address,
    const char* defaultResult) {
#define GETFAMILYNAMEFROM_IMPL(Family) \
  case Family:                         \
    return #Family

  switch (address->sa_family) {
    GETFAMILYNAMEFROM_IMPL(AF_INET);
    GETFAMILYNAMEFROM_IMPL(AF_INET6);
    GETFAMILYNAMEFROM_IMPL(AF_UNIX);
    GETFAMILYNAMEFROM_IMPL(AF_UNSPEC);
    default:
      return defaultResult;
  }

#undef GETFAMILYNAMEFROM_IMPL
}

void Peer::setFromHostPort(const char* host, uint16_t port) {
  ScopedAddrInfo results(getAddrInfo(host, port, 0));
  setFromAddrInfo(results.info);
}

void Peer::setFromHostPort(const char* hostAndPort) {
  HostAndPort hp(hostAndPort, true);
  ScopedAddrInfo results(getAddrInfo(hp.host, hp.port, 0));
  setFromAddrInfo(results.info);
}

void Peer::setFromIpPort(const char* ip, uint16_t port) {
  ScopedAddrInfo results(getAddrInfo(ip, port, AI_NUMERICHOST));
  setFromAddrInfo(results.info);
}

void Peer::setFromIpPort(const char* ipAndPort) {
  HostAndPort hp(ipAndPort, true);
  ScopedAddrInfo results(getAddrInfo(hp.host, hp.port, AI_NUMERICHOST));
  setFromAddrInfo(results.info);
}

void Peer::setFromLocalPort(uint16_t port) {
  ScopedAddrInfo results(getAddrInfo(nullptr, port, AI_ADDRCONFIG));
  setFromLocalAddr(results.info);
}

void Peer::setFromLocalPort(const char* port) {
  ScopedAddrInfo results(getAddrInfo(nullptr, port, AI_ADDRCONFIG));
  setFromLocalAddr(results.info);
}

void Peer::setFromPeerAddress(int socket) {
  setFromSocket(socket, getpeername);
}

void Peer::setFromLocalAddress(int socket) {
  setFromSocket(socket, getsockname);
}

void Peer::setFromSockaddr(const struct sockaddr* addr) {
  family_ = addr->sa_family;
  switch (family_) {
    case AF_INET: {
      const sockaddr_in* v4addr = reinterpret_cast<const sockaddr_in*>(addr);
      addr_.ipV4Addr = IPAddressV4(v4addr->sin_addr);
      port_ = ntohs(((sockaddr_in*)addr)->sin_port);
      break;
    }
    case AF_INET6: {
      const sockaddr_in6* v6addr = reinterpret_cast<const sockaddr_in6*>(addr);
      addr_.ipV6Addr = IPAddressV6(*v6addr);
      port_ = ntohs(((sockaddr_in6*)addr)->sin6_port);
      break;
    }
    default:
      throw std::invalid_argument(
          "Peer::setFromSockaddr() called on non-ip address");
  }
}

void Peer::setFromSockaddr(const struct sockaddr* address, socklen_t addrlen) {
  // Check the length to make sure we can access address->sa_family
  if (addrlen <
      (offsetof(struct sockaddr, sa_family) + sizeof(address->sa_family))) {
    throw std::invalid_argument(
        "Peer::setFromSockaddr() called with length too short for a sockaddr");
  }

  if (address->sa_family == AF_INET) {
    if (addrlen < sizeof(struct sockaddr_in)) {
      throw std::invalid_argument(
          "Peer::setFromSockaddr() called "
          "with length too short for a sockaddr_in");
    }
    setFromSockaddr(address);
  } else if (address->sa_family == AF_INET6) {
    if (addrlen < sizeof(struct sockaddr_in6)) {
      throw std::invalid_argument(
          "Peer::setFromSockaddr() called "
          "with length too short for a sockaddr_in6");
    }
    setFromSockaddr(address);
  } else {
    throw std::invalid_argument(
        "Peer::setFromSockaddr() called with unsupported address type");
  }
}

socklen_t Peer::getAddress(sockaddr_storage* addr) const {
  return toSockaddrStorage(addr, htons(port_));
}

std::string Peer::getAddressStr() const {
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument("Can't get address str for non-ip address");
  }
  return isV4() ? addr_.ipV4Addr.str() : addr_.ipV6Addr.str();
}

void Peer::getAddressStr(char* buf, size_t buflen) const {
  auto ret = getAddressStr();
  size_t len = std::min(buflen - 1, ret.size());
  memcpy(buf, ret.data(), len);
  buf[len] = '\0';
}

std::string Peer::getHostStr() const {
    return getIpString(0);
}

uint16_t Peer::port() const {
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument("Peer::port() called on non-ip address");
  }
  return port_;
}

void Peer::setPort(uint16_t port) {
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument("Peer::setPort() called on non-ip address");
  }
  port_ = port;
}

std::string Peer::describe() const {
  switch (family_) {
    case AF_UNSPEC:
      return "<uninitialized address>";
    case AF_INET: {
      char buf[NI_MAXHOST + 16];
      getAddressStr(buf, sizeof(buf));
      size_t iplen = strlen(buf);
      snprintf(buf + iplen, sizeof(buf) - iplen, ":%" PRIu16, port());
      return buf;
    }
    case AF_INET6: {
      char buf[NI_MAXHOST + 18];
      buf[0] = '[';
      getAddressStr(buf + 1, sizeof(buf) - 1);
      size_t iplen = strlen(buf);
      snprintf(buf + iplen, sizeof(buf) - iplen, "]:%" PRIu16, port());
      return buf;
    }
    default: {
      char buf[64];
      snprintf(buf, sizeof(buf), "<unknown address family %d>", family_);
      return buf;
    }
  }
}

bool Peer::operator==(const Peer& other) const {
  if (family_ != other.family_) {
    return false;
  }
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument(
        "Peer: unsupported address family for comparison");
  }
  if (isV4()) {
    return addr_.ipV4Addr == other.addr_.ipV4Addr && port_ == other.port_;
  } else {
    return addr_.ipV6Addr == other.addr_.ipV6Addr && port_ == other.port_;
  }
}

bool Peer::operator<(const Peer& other) const {
  if (family_ != other.family_) {
    return family_ < other.family_;
  }
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument(
        "Peer: unsupported address family for comparing");
  }
  if (port_ != other.port_) {
    return port_ < other.port_;
  }
  if (isV4()) {
    return addr_.ipV4Addr < other.addr_.ipV4Addr;
  } else {
    return addr_.ipV6Addr < other.addr_.ipV6Addr;
  }
}

size_t Peer::hash() const {
  size_t seed = acc::hash::twang_mix64(family_);
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument(
        "Peer: unsupported address family for hashing");
  }
  size_t addr = isV4() ? addr_.ipV4Addr.hash() : addr_.ipV6Addr.hash();
  return acc::hash::hash_combine(seed, port_, addr);
}

struct addrinfo*
Peer::getAddrInfo(const char* host, uint16_t port, int flags) {
  // getaddrinfo() requires the port number as a string
  char portString[sizeof("65535")];
  snprintf(portString, sizeof(portString), "%" PRIu16, port);
  return getAddrInfo(host, portString, flags);
}

struct addrinfo*
Peer::getAddrInfo(const char* host, const char* port, int flags) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | flags;

  struct addrinfo* results;
  int error = getaddrinfo(host, port, &hints, &results);
  if (error != 0) {
    auto os = acc::to<std::string>(
        "Failed to resolve address for '", host, "': ",
        gai_strerror(error), " (error=", error, ")");
    throw std::system_error(error, std::generic_category(), os);
  }

  return results;
}

void Peer::setFromAddrInfo(const struct addrinfo* info) {
  setFromSockaddr(info->ai_addr, socklen_t(info->ai_addrlen));
}

void Peer::setFromLocalAddr(const struct addrinfo* info) {
  // Prefer IPv4
  for (const struct addrinfo* ai = info; ai != nullptr; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET) {
      setFromSockaddr(ai->ai_addr, socklen_t(ai->ai_addrlen));
      return;
    }
  }

  // Otherwise, just use the first address in the list.
  setFromSockaddr(info->ai_addr, socklen_t(info->ai_addrlen));
}

void Peer::setFromSocket(
    int socket,
    int (*fn)(int, struct sockaddr*, socklen_t*)) {
  // Try to put the address into a local storage buffer.
  sockaddr_storage tmp_sock;
  socklen_t addrLen = sizeof(tmp_sock);
  if (fn(socket, (sockaddr*)&tmp_sock, &addrLen) != 0) {
    acc::throwSystemError("setFromSocket() failed");
  }

  setFromSockaddr((sockaddr*)&tmp_sock, addrLen);
}

std::string Peer::getIpString(int flags) const {
  char addrString[NI_MAXHOST];
  getIpString(addrString, sizeof(addrString), flags);
  return std::string(addrString);
}

void Peer::getIpString(char* buf, size_t buflen, int flags) const {
  if (UNLIKELY(!isFamilyInet())) {
    throw std::invalid_argument(
        "Peer: attempting to get IP address for a non-ip address");
  }

  sockaddr_storage tmp_sock;
  toSockaddrStorage(&tmp_sock, port_);
  int rc = getnameinfo(
      (sockaddr*)&tmp_sock,
      sizeof(sockaddr_storage),
      buf,
      buflen,
      nullptr,
      0,
      flags);
  if (rc != 0) {
    auto os = acc::to<std::string>(
        "getnameinfo() failed in getIpString() error = ", gai_strerror(rc));
    throw std::system_error(rc, std::generic_category(), os);
  }
}

int Peer::toSockaddrStorage(sockaddr_storage* dest, uint16_t port) const {
  memset(dest, 0, sizeof(sockaddr_storage));
  dest->ss_family = family_;

  if (isV4()) {
    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(dest);
    sin->sin_addr = addr_.ipV4Addr.toAddr();
    sin->sin_port = port;
#if defined(__APPLE__)
    sin->sin_len = sizeof(*sin);
#endif
    return sizeof(*sin);
  } else if (isV6()) {
    sockaddr_in6* sin = reinterpret_cast<sockaddr_in6*>(dest);
    sin->sin6_addr = addr_.ipV6Addr.toAddr();
    sin->sin6_port = port;
    sin->sin6_scope_id = addr_.ipV6Addr.scopeId();
#if defined(__APPLE__)
    sin->sin6_len = sizeof(*sin);
#endif
    return sizeof(*sin);
  } else {
    throw std::invalid_argument(
        "Peer: attempting to store IP address for a non-ip address");
  }
}

} // namespace raster
