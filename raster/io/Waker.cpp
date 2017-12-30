/*
 * Copyright (C) 2017, Yeolar
 */

#include <fcntl.h>
#include <unistd.h>

#include "raster/io/FileUtil.h"
#include "raster/io/Waker.h"
#include "raster/util/Logging.h"

namespace rdd {

Waker::Waker() {
  role_ = kWaker;
  if (::pipe2(pipeFds_, O_CLOEXEC | O_NONBLOCK) == -1) {
    RDDPLOG(ERROR) << "pipe2 failed";
  }
}

void Waker::wake() const {
  writeNoInt(pipeFds_[1], (void*)"x", 1);
  RDDLOG(V2) << *this << " wake";
}

void Waker::consume() const {
  char c;
  while (readNoInt(pipeFds_[0], &c, 1) > 0) {}
  RDDLOG(V2) << *this << " consume";
}

void Waker::close() {
  ::close(pipeFds_[0]);
  ::close(pipeFds_[1]);
}

std::ostream& operator<<(std::ostream& os, const Waker& waker) {
  os << "Waker(" << waker.fd() << ":" << waker.fd2() << ")";
  return os;
}

} // namespace rdd
