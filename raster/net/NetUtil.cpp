/*
 * Copyright (C) 2017, Yeolar
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "raster/net/NetUtil.h"
#include "raster/util/Logging.h"

namespace rdd {

std::string getAddr(const std::string& ifname) {
  std::string addr;
  struct ifaddrs *ifaddr, *ifa;
  char ni_host[NI_MAXHOST];
  int r = getifaddrs(&ifaddr);
  if (r == -1) {
    RDDPLOG(ERROR) << "getifaddrs failed";
    return addr;
  }
  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr != nullptr &&
        ifa->ifa_name == ifname &&
        ifa->ifa_addr->sa_family == AF_INET) {
      r = getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in),
                      ni_host, NI_MAXHOST,
                      nullptr, 0,
                      NI_NUMERICHOST);
      if (r != 0) {
        RDDLOG(ERROR) << "getnameinfo failed, " << gai_strerror(r) << ", "
          << (r == EAI_SYSTEM ? strerror(errno) : "");
      } else {
        addr = ni_host;
      }
      break;
    }
  }
  freeifaddrs(ifaddr);
  return addr;
}

std::string getNodeName(bool trimSuffix) {
  struct utsname buf;
  if (uname(&buf) != -1) {
    char* s = buf.nodename;
    char* p = strchr(s, '.');
    return trimSuffix && p ? std::string(s, p - s) : s;
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

// TODO prefer to use getnameinfo
std::string ipv4ToHost(const std::string& ip, bool trimSuffix) {
  struct in_addr addr;
  if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
    return ip + "(failed to get host)";
  }
  struct hostent *he = gethostbyaddr(&addr, sizeof(addr), AF_INET);
  if (!he) {
    return ip + "(failed to get host)";
  }
  char* s = he->h_name;
  char* p = strchr(s, '.');
  return trimSuffix && p ? std::string(s, p - s) : s;
}

bool isValidIP(const std::string& ip) {
  struct addrinfo *ai, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(ip.c_str(), 0, &hints, &ai) != 0) {
    return false;
  }
  return ai != nullptr;
}

bool isValidPort(uint16_t port) {
  return port > 0;
}

} // namespace rdd
