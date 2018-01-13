/*
 * Copyright 2017 Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
