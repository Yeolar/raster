/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

#include "raster/net/Peer.h"

namespace rdd {

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

bool isValidIP(const std::string& ip);

bool isValidPort(uint16_t port);

} // namespace rdd
