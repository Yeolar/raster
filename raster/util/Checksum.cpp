/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Checksum.h"
#include <boost/crc.hpp>

namespace rdd {

uint32_t crc32(const uint8_t *data, size_t nbytes) {
  boost::crc_32_type sum;
  sum.process_bytes(data, nbytes);
  return sum.checksum();
}

} // namespace rdd
