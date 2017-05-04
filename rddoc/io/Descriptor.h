/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

namespace rdd {

class Descriptor {
public:
  virtual int fd() const = 0;
  virtual int role() const = 0;
  virtual char roleLabel() const = 0;
  virtual std::string str() = 0;
};

}

