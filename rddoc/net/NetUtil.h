/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <boost/operators.hpp>
#include "rddoc/util/Hash.h"
#include "rddoc/util/String.h"

namespace rdd {

struct Peer : public boost::totally_ordered<Peer> {
  std::string host;
  int port{0};

  Peer() {}
  Peer(const std::string& h, int p) : host(h), port(p) {}

  explicit Peer(const std::string& s) {
    split(':', trimWhitespace(s), host, port);
  }
  explicit Peer(int fd);

  std::string str() const {
    return to<std::string>(host, ':', port);
  }
};

inline bool operator==(const Peer& lhs, const Peer& rhs) {
  return lhs.host == rhs.host && lhs.port == rhs.port;
}

inline bool operator<(const Peer& lhs, const Peer& rhs) {
  return lhs.port < rhs.port || (lhs.port == rhs.port && lhs.host < rhs.host);
}

struct TimeoutOption {
  uint64_t ctimeout;    // connect timeout
  uint64_t rtimeout;    // read timeout
  uint64_t wtimeout;    // write timeout
};

inline std::ostream& operator<<(std::ostream& os,
                                const TimeoutOption& timeoutOpt) {
  os << "{" << timeoutOpt.ctimeout
     << "," << timeoutOpt.rtimeout
     << "," << timeoutOpt.wtimeout << "}";
  return os;
}

struct ClientOption {
  Peer peer;
  TimeoutOption timeout;
};

std::string getAddr(const std::string& ifname);

std::string getNodeName(bool trimSuffix = false);

std::string getNodeIp();

std::string ipv4ToHost(const std::string& ip, bool trimSuffix = false);

} // namespace rdd

namespace std {

template <>
struct hash<rdd::Peer> {
  size_t operator()(const rdd::Peer& p) const {
    return rdd::hash::hash_combine(p.host, p.port);
  }
};

} // namespace std
