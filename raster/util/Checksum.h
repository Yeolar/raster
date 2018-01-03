/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace rdd {

uint32_t crc32c(const uint8_t* data, size_t nbytes,
                uint32_t startingChecksum = ~0U);

uint32_t crc32(const uint8_t* data, size_t nbytes,
               uint32_t startingChecksum = ~0U);

/*
 * compared to crc32(), crc32_type() uses a different set of default
 * parameters to match the results returned by boost::crc_32_type and
 * php's built-in crc32 implementation
 */
uint32_t crc32_type(const uint8_t* data, size_t nbytes,
                    uint32_t startingChecksum = ~0U);

} // namespace rdd
