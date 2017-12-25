/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Descriptor.h"

#define RDD_IO_DESCRIPTOR_STR(role) #role

namespace {
  static const char* roleStrings[] = {
    RDD_IO_DESCRIPTOR_GEN(RDD_IO_DESCRIPTOR_STR)
  };
}

namespace rdd {

const char* Descriptor::roleName() const {
  return roleStrings[role_];
}

} // namespace rdd
