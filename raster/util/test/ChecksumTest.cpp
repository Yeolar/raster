/*
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

#include "raster/util/Checksum.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(crc32c, all) {
  // leveldb crc32c
  EXPECT_EQ(3703331322, crc32c((uint8_t*)"TestCRCBuffer", 13) ^ 0xffffffff);
}

TEST(crc32_type, all) {
  EXPECT_EQ(0,          crc32_type((uint8_t*)"", 0));
  EXPECT_EQ(4108050209, crc32_type((uint8_t*)"0", 1));
  EXPECT_EQ(3477152822, crc32_type((uint8_t*)"01", 2));
  EXPECT_EQ(3584060080, crc32_type((uint8_t*)"012", 3));
  EXPECT_EQ(2791742845, crc32_type((uint8_t*)"0123", 4));
  EXPECT_EQ(3718541348, crc32_type((uint8_t*)"01234", 5));
  EXPECT_EQ(3094309647, crc32_type((uint8_t*)"012345", 6));
  EXPECT_EQ(2378107118, crc32_type((uint8_t*)"0123456", 7));
  EXPECT_EQ(763378421,  crc32_type((uint8_t*)"01234567", 8));
  EXPECT_EQ(939184570,  crc32_type((uint8_t*)"012345678", 9));
  EXPECT_EQ(2793719750, crc32_type((uint8_t*)"0123456789", 10));
}
