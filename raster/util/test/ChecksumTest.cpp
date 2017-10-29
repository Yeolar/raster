/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Checksum.h"
#include <gtest/gtest.h>

using namespace rdd;

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
