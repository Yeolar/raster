/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Checksum.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(crc32, all) {
  EXPECT_EQ(0,          crc32(""));
  EXPECT_EQ(4108050209, crc32("0"));
  EXPECT_EQ(3477152822, crc32("01"));
  EXPECT_EQ(3584060080, crc32("012"));
  EXPECT_EQ(2791742845, crc32("0123"));
  EXPECT_EQ(3718541348, crc32("01234"));
  EXPECT_EQ(3094309647, crc32("012345"));
  EXPECT_EQ(2378107118, crc32("0123456"));
  EXPECT_EQ(763378421,  crc32("01234567"));
  EXPECT_EQ(939184570,  crc32("012345678"));
  EXPECT_EQ(2793719750, crc32("0123456789"));
}
