/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

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

  virtual ~Descriptor() {}

  virtual int fd() const = 0;

  Role role() const { return role_; }
  const char* roleName() const;

 protected:
  Role role_{kNone};
};

#undef RDD_IO_DESCRIPTOR_ENUM

} // namespace rdd
