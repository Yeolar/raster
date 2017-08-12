/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <unistd.h>

namespace rdd {

class BPTreeWriter {
public:
  BPTreeWriter(const std::string& file) {
    fd_ = open
  }

  ~BPTreeWriter() {
    close();
  }

  void close() {
    if (fd_ != 0) {
      ::close(fd_);
      fd_ = 0;
    }
  }

private:
  int fd_{0};
  std::string file_;
  size_t fileSize_;
};

} // namespace rdd
