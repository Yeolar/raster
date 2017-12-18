/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPHeaders.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(headers, all) {
  const char* data =
    "Foo: bar\r\n"
    " baz\r\n"
    "Asdf: qwer\r\n"
    "\tzxcv\r\n"
    "Foo: even\r\n"
    "     more\r\n"
    "     lines\r\n";

  HTTPHeaders headers;
  headers.parse(data);
  EXPECT_EQ(3, headers.size());
  EXPECT_EQ(1, headers.getNumberOfValues("asdf"));
  EXPECT_STREQ("qwer zxcv", headers.combine("asdf").c_str());
  EXPECT_STREQ("qwer zxcv", headers.getSingleOrEmpty("asdf").c_str());
  EXPECT_EQ(2, headers.getNumberOfValues("Foo"));
  EXPECT_STREQ("bar baz, even more lines", headers.combine("Foo").c_str());
  EXPECT_TRUE(headers.getSingleOrEmpty("Foo").empty());
}

