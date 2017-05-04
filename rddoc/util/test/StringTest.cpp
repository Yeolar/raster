/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/String.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(split, all) {
  std::string s = "tair_db::tair_test::id";
  std::vector<std::string> v;
  split("::", s, v);
  EXPECT_EQ("tair_db", v[0]);
  EXPECT_EQ("tair_test", v[1]);
  EXPECT_EQ("id", v[2]);
}
