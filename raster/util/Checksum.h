/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstddef>
#include <string>

namespace rdd {

uint32_t crc32(const uint8_t* data, size_t nbytes);

inline uint32_t crc32(const std::string& data) {
  return crc32((const uint8_t*)data.data(), data.size());
}

} // namespace rdd
