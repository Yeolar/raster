/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <unistd.h>
#include <fcntl.h>
#include "rddoc/io/Descriptor.h"
#include "rddoc/util/Logging.h"

namespace rdd {

class Waker : public Descriptor {
public:
  Waker() {
    if (pipe2(pipefd_, O_CLOEXEC | O_NONBLOCK) == -1) {
      RDDPLOG(ERROR) << "pipe2 failed";
    }
  }

  ~Waker() {
    close();
  }

  virtual int fd() const { return pipefd_[0]; }
  virtual int role() const { return -1; }
  virtual char roleLabel() const { return 'W'; }
  virtual std::string str() { return "waker"; }

  void wake() {
    write(pipefd_[1], (void*)"x", 1);
    RDDLOG(V2) << "waker wake";
  }

  void consume() {
    char c;
    while (read(pipefd_[0], &c, 1) > 0) {}
    RDDLOG(V2) << "waker consume";
  }

private:
  void close() {
    ::close(pipefd_[0]);
    ::close(pipefd_[1]);
  }

  int pipefd_[2];
};

}

