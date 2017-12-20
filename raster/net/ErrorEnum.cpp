/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/ErrorEnum.h"

#define RDD_NET_ERROR_STR(error) #error

namespace {
  static const char* netErrorStrings[] = {
    RDD_NET_ERROR_GEN(RDD_NET_ERROR_STR)
  };
}

namespace rdd {

const char* getNetErrorString(NetError error) {
  if (error < kErrorNone || error >= kErrorMax) {
    return netErrorStrings[kErrorMax];
  } else {
    return netErrorStrings[error];
  }
}

const char* getNetErrorStringByIndex(int i) {
  return netErrorStrings[i];
}

} // namespace rdd
