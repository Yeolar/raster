/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Demangle.h"

#include <cstring>
#include <cxxabi.h>

namespace rdd {

std::string demangle(const char* name) {
  int status;
  size_t len = 0;
  // malloc() memory for the demangled type name
  char* demangled = abi::__cxa_demangle(name, nullptr, &len, &status);
  if (status != 0) {
    return name;
  }
  return std::string(demangled, strlen(demangled));
}

} // namespace rdd
