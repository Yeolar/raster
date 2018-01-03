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

std::ostream& operator<<(std::ostream& os, const TimeoutOption& timeout);

struct ClientOption {
  Peer peer;
  TimeoutOption timeout;
};

std::string getNodeName();

std::string getNodeIp();

} // namespace rdd
