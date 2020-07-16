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

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace raster {

typedef std::array<uint8_t, 4> ByteArray4;
typedef std::array<uint8_t, 16> ByteArray16;

class IPAddressV4 {
 public:
  IPAddressV4() {}
  IPAddressV4(const in_addr src) noexcept : addr_(src) {}
  IPAddressV4(const ByteArray4& src) noexcept : addr_(src) {}

  in_addr toAddr() const { return addr_.inAddr_; }
  uint32_t toLong() const { return toAddr().s_addr; }
  uint32_t toLongHBO() const { return ntohl(toLong()); }

  std::string str() const;
  std::string toFullyQualified() const { return str(); }
  void toFullyQualifiedAppend(std::string& out) const;

  bool operator==(const IPAddressV4& other) const {
    return (toLong() == other.toLong());
  }
  bool operator<(const IPAddressV4& other) const {
    return (toLongHBO() < other.toLongHBO());
  }

  size_t hash() const;

 private:
  union AddressStorage {
    in_addr inAddr_;
    ByteArray4 bytes_;
    AddressStorage() {
      ::memset(this, 0, sizeof(AddressStorage));
    }
    explicit AddressStorage(const in_addr addr) : inAddr_(addr) {}
    explicit AddressStorage(const ByteArray4 bytes) : bytes_(bytes) {}
  } addr_;
};

class IPAddressV6 {
 public:
  IPAddressV6() {}
  IPAddressV6(const in6_addr& src) noexcept : addr_(src) {}
  IPAddressV6(const ByteArray16& src) noexcept : addr_(src) {}
  IPAddressV6(const sockaddr_in6& src) noexcept
    : addr_(src.sin6_addr), scope_(uint16_t(src.sin6_scope_id)) {}

  in6_addr toAddr() const { return addr_.in6Addr_; }
  uint16_t scopeId() const { return scope_; }
  const unsigned char* bytes() const { return addr_.in6Addr_.s6_addr; }

  bool isIPv4Mapped() const;

  IPAddressV4 createIPv4() const;

  std::string str() const;
  std::string toFullyQualified() const;
  void toFullyQualifiedAppend(std::string& out) const;

  bool operator==(const IPAddressV6& other) const {
    return tie() == other.tie();
  }
  bool operator<(const IPAddressV6& other) const {
    return tie() < other.tie();
  }

  size_t hash() const;

 private:
  std::tuple<ByteArray16, uint16_t> tie() const {
    return std::tie(addr_.bytes_, scope_);
  }

  union AddressStorage {
    in6_addr in6Addr_;
    ByteArray16 bytes_;
    AddressStorage() {
      ::memset(this, 0, sizeof(AddressStorage));
    }
    explicit AddressStorage(const in6_addr& addr) : in6Addr_(addr) {}
    explicit AddressStorage(const ByteArray16& bytes) : bytes_(bytes) {}
  } addr_;
  uint16_t scope_{0};
};

class Peer {
 public:
  static int getPortFrom(const struct sockaddr* address);

  static const char* getFamilyNameFrom(
      const struct sockaddr* address,
      const char* defaultResult = nullptr);

  Peer() {}

  Peer(const char* host, uint16_t port, bool allowNameLookup = false) {
    if (allowNameLookup) {
      setFromHostPort(host, port);
    } else {
      setFromIpPort(host, port);
    }
  }

  Peer(const std::string& host, uint16_t port, bool allowNameLookup = false) {
    if (allowNameLookup) {
      setFromHostPort(host.c_str(), port);
    } else {
      setFromIpPort(host.c_str(), port);
    }
  }

  Peer(const Peer&) = default;
  Peer& operator=(const Peer&) = default;
  Peer(Peer&&) = default;
  Peer& operator=(Peer&&) = default;

  void setFromHostPort(const char* host, uint16_t port);
  void setFromHostPort(const std::string& host, uint16_t port) {
    setFromHostPort(host.c_str(), port);
  }
  void setFromHostPort(const char* hostAndPort);
  void setFromHostPort(const std::string& hostAndPort) {
    setFromHostPort(hostAndPort.c_str());
  }

  void setFromIpPort(const char* ip, uint16_t port);
  void setFromIpPort(const std::string& ip, uint16_t port) {
    setFromIpPort(ip.c_str(), port);
  }
  void setFromIpPort(const char* ipAndPort);
  void setFromIpPort(const std::string& ipAndPort) {
    setFromIpPort(ipAndPort.c_str());
  }

  void setFromLocalPort(uint16_t port);
  void setFromLocalPort(const char* port);
  void setFromLocalPort(const std::string& port) {
    setFromLocalPort(port.c_str());
  }

  void setFromPeerAddress(int socket);
  void setFromLocalAddress(int socket);

  void setFromSockaddr(const struct sockaddr* address);
  void setFromSockaddr(const struct sockaddr* address, socklen_t addrlen);

  sa_family_t family() const { return family_; }

  bool isInitialized() const { return family_ != AF_UNSPEC; }
  bool empty() const { return family_ == AF_UNSPEC; }
  bool isV4() const { return family_ == AF_INET; }
  bool isV6() const { return family_ == AF_INET6; }

  bool isFamilyInet() const {
    return family_ == AF_INET || family_ == AF_INET6;
  }

  socklen_t getAddress(sockaddr_storage* addr) const;

  std::string getAddressStr() const;
  void getAddressStr(char* buf, size_t buflen) const;

  std::string getHostStr() const;

  uint16_t port() const;
  void setPort(uint16_t port);

  std::string describe() const;

  bool operator==(const Peer& other) const;
  bool operator!=(const Peer& other) const { return !(*this == other); }

  bool operator<(const Peer& other) const;

  size_t hash() const;

 private:
  struct addrinfo* getAddrInfo(const char* host, uint16_t port, int flags);
  struct addrinfo* getAddrInfo(const char* host, const char* port, int flags);
  void setFromAddrInfo(const struct addrinfo* results);
  void setFromLocalAddr(const struct addrinfo* results);
  void setFromSocket(int socket, int (*fn)(int, struct sockaddr*, socklen_t*));
  std::string getIpString(int flags) const;
  void getIpString(char* buf, size_t buflen, int flags) const;
  int toSockaddrStorage(sockaddr_storage* dest, uint16_t port) const;

  typedef union IPAddressV46 {
    IPAddressV4 ipV4Addr;
    IPAddressV6 ipV6Addr;
    // default constructor
    IPAddressV46() noexcept {
      ::memset(this, 0, sizeof(IPAddressV46));
    }
    explicit IPAddressV46(const IPAddressV4& addr) noexcept : ipV4Addr(addr) {}
    explicit IPAddressV46(const IPAddressV6& addr) noexcept : ipV6Addr(addr) {}
  } IPAddressV46;
  IPAddressV46 addr_;
  sa_family_t family_;
  uint16_t port_;
};

inline std::ostream& operator<<(std::ostream& os, const Peer& peer) {
  os << peer.describe();
  return os;
}

} // namespace raster

namespace std {

template <>
struct hash<raster::Peer> {
  size_t operator()(const raster::Peer& peer) const {
    return peer.hash();
  }
};

} // namespace std
