/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/NetUtil.h"

#include <sys/utsname.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "raster/util/Logging.h"

namespace rdd {

std::ostream& operator<<(std::ostream& os, const TimeoutOption& timeout) {
  os << "{" << timeout.ctimeout
     << "," << timeout.rtimeout
     << "," << timeout.wtimeout << "}";
  return os;
}

std::string getNodeName() {
  struct utsname buf;
  if (uname(&buf) != -1) {
    return buf.nodename;
  }
  return "(unknown)";
}

std::string getNodeIp() {
  static std::string ip;
  if (ip.empty()) {
    struct hostent* ent = gethostbyname(getNodeName().c_str());
    if (ent != nullptr) {
      switch (ent->h_addrtype) {
        case AF_INET:
        case AF_INET6:
          {
            char str[128];
            inet_ntop(ent->h_addrtype, ent->h_addr, str, sizeof(str));
            ip = str;
          }
          break;
        default: break;
      }
    }
  }
  return ip.empty() ? "(unknown)" : ip;
}

} // namespace rdd
