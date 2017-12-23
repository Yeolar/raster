/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/Poll.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(Poll_Event, ostream) {
  std::ostringstream os;
  os << Poll::Event(5);
  EXPECT_STREQ("(EPOLLIN|EPOLLOUT)", os.str().c_str());
}
