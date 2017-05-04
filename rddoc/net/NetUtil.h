/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include "rddoc/util/String.h"

namespace rdd {

struct Peer {
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

struct TimeoutOption {
  uint64_t ctimeout{0};   // connect timeout
  uint64_t rtimeout{0};   // read timeout
  uint64_t wtimeout{0};   // write timeout
};

inline std::ostream& operator<<(std::ostream& os,
                                const TimeoutOption& timeout_opt) {
  os << "{" << timeout_opt.ctimeout
     << "," << timeout_opt.rtimeout
     << "," << timeout_opt.wtimeout << "}";
  return os;
}

struct ClientOption {
  Peer peer;
  TimeoutOption timeout;
};

std::string getAddr(const std::string& ifname);

std::string getNodeName(bool trim_suffix = false);

std::string getNodeIp();

std::string ipv4ToHost(const std::string& ip, bool trim_suffix = false);

}

