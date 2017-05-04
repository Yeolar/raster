/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/FixedStream.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(FixedOstream, all) {
  char buf[256];
  FixedOstream stream(buf, sizeof(buf));

  EXPECT_EQ(buf, stream.output());
  EXPECT_EQ(buf, stream.output_ptr());
  EXPECT_EQ(buf + sizeof(buf), stream.output_end());
  EXPECT_EQ("", stream.str());

  stream << "abcdefg" << 1234567 << 'x';
  EXPECT_EQ("abcdefg1234567x", stream.str());

  stream.reset();

  EXPECT_EQ(buf, stream.output());
  EXPECT_EQ(buf, stream.output_ptr());
  EXPECT_EQ(buf + sizeof(buf), stream.output_end());
  EXPECT_EQ("", stream.str());
}
