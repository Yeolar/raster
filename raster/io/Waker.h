/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <iostream>

#include "raster/io/Descriptor.h"

namespace rdd {

class Waker : public Descriptor {
 public:
  Waker();

  ~Waker() override {
    close();
  }

  int fd() const override {
    return pipeFds_[0];
  }

  int fd2() const {
    return pipeFds_[1];
  }

  void wake() const;

  void consume() const;

 private:
  void close();

  int pipeFds_[2];
};

std::ostream& operator<<(std::ostream& os, const Waker& waker);

} // namespace rdd
