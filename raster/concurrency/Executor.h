/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/util/Function.h"

namespace rdd {

/**
 * An Executor accepts units of work with add(), which should be threadsafe.
 */
class Executor {
 public:
  virtual ~Executor() {}
  virtual void add(VoidFunc) = 0;
};

} // namespace rdd
