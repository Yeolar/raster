/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

#include "raster/net/NetUtil.h"

namespace rdd {

#define RDD_IO_DESCRIPTOR_GEN(x)  \
    x(None),                      \
    x(Waker),                     \
    x(Listener),                  \
    x(Server),                    \
    x(Client)

#define RDD_IO_DESCRIPTOR_ENUM(role) k##role

class Descriptor {
public:
  enum Role {
    RDD_IO_DESCRIPTOR_GEN(RDD_IO_DESCRIPTOR_ENUM)
  };

  Descriptor(Role role) : role_(role) {}

  virtual int fd() const = 0;
  virtual Peer peer() = 0;

  const char* role() const;

protected:
  Role role_;
};

inline std::ostream& operator<<(std::ostream& os, Descriptor& d) {
  os << d.role() << ":" << d.fd() << "[" << d.peer() << "]";
  return os;
}

#undef RDD_IO_DESCRIPTOR_ENUM

} // namespace rdd
