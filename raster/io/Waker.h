/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <unistd.h>
#include <fcntl.h>

#include "raster/io/Descriptor.h"
#include "raster/util/Exception.h"
#include "raster/util/Logging.h"

namespace rdd {

class Waker : public Descriptor {
public:
  Waker() {
    role_ = kWaker;
    if (pipe2(pipeFds_, O_CLOEXEC | O_NONBLOCK) == -1) {
      RDDPLOG(ERROR) << "pipe2 failed";
    }
  }

  ~Waker() {
    close();
  }

  virtual int fd() const { return pipeFds_[0]; }

  void wake() {
    checkUnixError(write(pipeFds_[1], (void*)"x", 1), "write error");
    RDDLOG(V2) << "waker wake";
  }

  void consume() {
    char c;
    while (read(pipeFds_[0], &c, 1) > 0) {}
    RDDLOG(V2) << "waker consume";
  }

private:
  void close() {
    ::close(pipeFds_[0]);
    ::close(pipeFds_[1]);
  }

  int pipeFds_[2];
};

inline std::ostream& operator<<(std::ostream& os, const Waker& waker) {
  os << waker.roleName()[0] << ":" << waker.fd();
  return os;
}

} // namespace rdd
