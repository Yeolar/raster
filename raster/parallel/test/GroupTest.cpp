/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/parallel/Group.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(Group, all) {
  Group group;

  for (size_t i = 1; i < 200; i++) {
    EXPECT_EQ(i, group.create(i));
    EXPECT_EQ(i, group.count());
  }
  for (size_t i = 1; i < 200; i++) {
    for (size_t j = i; j > 1; j--) {
      EXPECT_FALSE(group.finish(i));
    }
    EXPECT_TRUE(group.finish(i));
  }
}
